/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glviewport.h"
#include "shading.h"
#include "voxelaggregate.h"
#include "voxelscene.h"
#include "edittool.h"
#include "util/shaderinfo.h"
#include <iostream>
#include <cmath>
#include <QSurfaceFormat>
#include <QOpenGLShaderProgram>
#include <QMouseEvent>
//#include <QMatrix4x4>
#include <QDebug>

float GlViewportWidget::sRGB_LUT[1024];

/* ========== GlViewportSettings ==============*/
/* QMatrix4x4().perspective(...) * QMatrix().lookAt(...);
   is equivalent to
   ( QMatrix4x4().perspective(...) ).lookAt(...);
*/

QMatrix4x4 ViewportSettings::getGlMatrix() const
{
	return proj * view;
}

ray_t ViewportSettings::unproject(const QVector3D &pNear) const
{
	// TODO: cache matrices in class
	// pNear shall be on the near plane (z = 0), and the ray goes til the far plane (z = 1)
	// in viewport coordinates (not NDC!); make sure to adapt if z-mapping is not [0, 1]
	QVector3D pFar = QVector3D(pNear[0], pNear[1], 1.0);
	ray_t ray;
	QMatrix4x4 port;
	QMatrix4x4 glMatrix = getGlMatrix();
	port.viewport(0, 0, parent->width(), parent->height());
	QMatrix4x4 invProj = (port * glMatrix).inverted();
	ray.from = invProj * pNear;
	ray.dir = invProj * pFar - ray.from;
	ray.t_min = 0;
	ray.t_max = ray.dir.length();
	ray.dir /= ray.t_max;
	//qDebug() << camPivot << "\n ray.from:" << ray.from << "\n ray.dir:" << ray. dir << "\n t_max:" << ray.t_max;
	return ray;
}

void ViewportSettings::rotateBy(float dHead, float dPitch)
{
	heading += 0.5f * dHead;
	pitch += 0.5f * dPitch;
	while (heading < 0) heading += 360;
	while (heading > 360) heading -= 360;
	while (pitch < -180) pitch += 360;
	while (pitch > 180) pitch -= 360;
	updateViewMatrix();
}

void ViewportSettings::panBy(float dX, float dY)
{
	float zoom = camDistance / 20.f;
	QVector4D offset = dX * view.row(0) + dY * view.row(1);
	camPivot += zoom * QVector3D(offset);
	updateViewMatrix();
}

void ViewportSettings::zoomBy(float dZ)
{
	float steps = dZ / 120.f;
	float stepSize = std::max(0.5, 0.25 * floor(camDistance/4));
	camDistance -= steps * stepSize;
	if (camDistance < 1)
		camDistance = 1;
	else if (camDistance > 100)
		camDistance = 100;
	updateViewMatrix();
}

void ViewportSettings::updateViewport()
{
	proj.setToIdentity();
	proj.perspective(fov, (float)parent->width()/(float)parent->height(), 0.5, 1000);
}

void ViewportSettings::updateViewMatrix()
{
	view.setToIdentity();
	view.translate(0, 0, -camDistance);
	QMatrix4x4 rot;
	rot.rotate(QQuaternion::fromEulerAngles(pitch, heading, roll));
	rot = rot.transposed();
	rot.translate(-camPivot);
	view *= rot;
}

/* ========== GlViewportWidget ==============*/

GlViewportWidget::GlViewportWidget(VoxelScene *pscene, QWidget *parent):
	QOpenGLWidget(parent), scene(pscene), tesselationChanged(false), showGrid(true), dragStatus(DRAG_NONE), currentTool(0)
{
	scene->viewport = this;  // TODO: think about a nicer way...
}

void GlViewportWidget::generateUBOs()
{
	m_ubo_LUT = genVertexUBO(*this);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo_LUT);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// material properties for trove
	static const float materialProps[10][4] =
	{
		/* spec_amount; spec_sharpness; spec_tinting; emit; */
		{ 0.2, 10, 0.7, 0.0 }, // solid, rough
		{ 1.5, 50, 1.0, 0.0 }, // solid, metal
		{ 0.3, 100, 0.5, 0.0 }, // solid, water, sharper spec than metal, same as glass
		{ 0.3, 30, 0.5, 0.0 }, // solid, iridescent (not implemented yet)
		{ 0.3, 30, 0.5, 0.0 }, // solid, wave ???
		{ 0.3, 20, 0.5, 0.0 }, // solid, waxy ???
		{ 0.3, 10, 0.5, 0.98 }, // glowing solid
		{ 0.3, 75, 0.7, 0.0 }, // glass
		{ 0.3, 75, 0.7, 0.0 }, // tiled glass (redundant?)
		{ 0.3, 75, 0.7, 0.9 }, // glowing glass, unsure about specular
	};
	glGenBuffers(1, &m_ubo_material);
	glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_material);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(materialProps), materialProps, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_ubo_material);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GlViewportWidget::setSamples(int numSamples)
{
	QSurfaceFormat fmt = format();
	fmt.setSamples(numSamples);
	setFormat(fmt);
}

void GlViewportWidget::initializeGL()
{
	initializeOpenGLFunctions();
#if DEBUG_GL
	glDebug = new QOpenGLDebugLogger(this);
	if (glDebug->initialize())
		std::cout << "OpenGL debug logging enabled!\n";
	int32_t nmsg;
	glGetIntegerv(GL_MAX_DEBUG_LOGGED_MESSAGES, &nmsg);
	std::cout << "log limit: " << nmsg << " messages\n";
#endif
	glClearColor(0.2f, 0.3f, 0.35f, 1.0f);
	// debug
	QSurfaceFormat fmt = format();
	std::cout << "format depth buffer: " << fmt.depthBufferSize() << " GL Ver.: " << fmt.majorVersion() << "."
			<< fmt.minorVersion()
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
			<< " Color Space: " << fmt.colorSpace() /* Qt 5.10+ */
#endif
			<< std::endl;

	generateUBOs();
	// load shaders
	initShaders(*this);
	m_normal_tex = genNormalTex(*this);

	QOpenGLShaderProgram* flatProgram = getShaderProgram(SHADER_FLAT_COLOR);
	flatProgram->bind(); // unnecessary?
	// test: print UBO layout info
	//GLInfoLib::getUniformsInfo(flatProgram->programId());
	// connect sRGB UBO
	GLuint block_index = glGetUniformBlockIndex(flatProgram->programId(), "sRGB_LUT");
	glUniformBlockBinding(flatProgram->programId(), block_index, 0);

	// static object buffers
	WireCube::initializeStaticGL(*this);

	boundCube = new WireCube;
	boundCube->setShape(scene->layers[scene->activeLayerN]->bound);
	boundCube->setColor(rgba_t(255, 160, 160, 255));

	// line grid
	grid = new LineGrid();
	grid->setShape(1, IBBox(IVector3D(-32, 0, -32), IVector3D(32, 0, 32)));

	glEnable(GL_DEPTH_TEST);

	vpSettings = new ViewportSettings(this);

#if DEBUG_GL
	QDebug dbg = qDebug();
	QList<QOpenGLDebugMessage> msgList = glDebug->loggedMessages();
	for (auto &msg: msgList)
		if (msg.severity() < QOpenGLDebugMessage::NotificationSeverity)
			dbg << msg;
#endif
}
void GlViewportWidget::resizeGL(int w, int h)
{
	vpSettings->updateViewport();
}

void GlViewportWidget::paintGL()
{
	// TODO: scene->render() without scene->update() loses dirty info, so probably force call internally;
	// probably dirty and render object should be in viewport after all
	if (scene->needsUpdate())
		scene->update();
	if (tesselationChanged)
	{
		for (auto layer: scene->layers)
		{
			layer->renderAg->rebuild(*this, &renderOptions);
		}
		tesselationChanged = false;
	}
	// TODO: multiply with devicePixelRatio() or the devicePixelRatioF() for Qt 5.6+
	glViewport(0, 0, width(), height());
	glEnable(GL_FRAMEBUFFER_SRGB); // no effect prior to Qt 5.10+ (no way to request sRGB buffers)

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	QMatrix4x4 final = vpSettings->getGlMatrix();

	QOpenGLShaderProgram* flatProgram = getShaderProgram(SHADER_FLAT_COLOR);
	flatProgram->bind();
	flatProgram->setUniformValue("mvp_mat", final);
	glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_LUT);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo_LUT);
	// test object
	if (scene->layers[scene->activeLayerN]->useBound)
		boundCube->render(*this);
	if (showGrid)
		grid->render(*this);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	QOpenGLShaderProgram* voxelProgram = getShaderProgram(SHADER_VOXEL);
	voxelProgram->bind();
	voxelProgram->setUniformValue("mvp_mat", final);
	voxelProgram->setUniformValue("view_mat", vpSettings->getViewMatrix());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_normal_tex);
	//scene->renderLayer->render(*this);
	scene->render(*this);
	glDisable(GL_CULL_FACE);

#if DEBUG_GL
	QDebug dbg = qDebug();
	QList<QOpenGLDebugMessage> msgList = glDebug->loggedMessages();
	for (auto &msg: msgList)
		if (msg.severity() < QOpenGLDebugMessage::NotificationSeverity)
			dbg << msg;
#endif
}

GLRenderable* GlViewportWidget::getGrid()
{
	return grid;
}

void GlViewportWidget::setViewMode(RenderOptions::Modes mode)
{
	if (mode != renderOptions.mode)
	{
		renderOptions.mode = mode;
		tesselationChanged = true;
		// adjust grid
		IVector3D low(-32, -32, -32), high(32, 32, 32);
		if (mode == RenderOptions::MODE_FULL)
			low[renderOptions.axis] = high[renderOptions.axis] = 0;
		else
			low[renderOptions.axis] = high[renderOptions.axis] = renderOptions.level;
		grid->setShape(renderOptions.axis, IBBox(low, high));
		update();
	}
};

void GlViewportWidget::setShowGrid(bool enabled)
{
	if (enabled != showGrid)
	{
		showGrid = enabled;
		update();
	}
}

void GlViewportWidget::activeLayerChanged(int layerN)
{
	boundCube->setShape(scene->layers[scene->activeLayerN]->bound);
	update();
}

void GlViewportWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		dragStatus = DRAG_TOOL;
		if (currentTool)
		{
			ray_t ray = vpSettings->unproject(QVector3D(event->x(), height() - event->y(), 0.f));
			ToolEvent toolEvent(this, event, ray);
			currentTool->mouseDown(toolEvent);

			if (scene->needsUpdate())
			{
	//			scene->update();
				update();
			}
		}
	}
	if (event->button() == Qt::RightButton)
	{
		dragStatus = DRAG_ROTATE;
		dragStart = event->pos();
	}
	else if(event->button() == Qt::MiddleButton)
	{
		dragStatus = DRAG_PAN;
		dragStart = event->pos();
	}
	//update();
}

void GlViewportWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && currentTool)
	{
		// TODO: cache ray; currently don't need ray hit yet
		ray_t ray = vpSettings->unproject(QVector3D(event->x(), height() - event->y(), 0.f));
		ToolEvent toolEvent(this, event, ray);
		currentTool->mouseUp(toolEvent);
		if (scene->needsUpdate())
		{
			update();
		}
	}
	dragStatus = DRAG_NONE;
}


void GlViewportWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (dragStatus == DRAG_NONE)
		return;

	QPoint delta = event->pos() - dragStart;
	if (dragStatus == DRAG_ROTATE)
	{
		vpSettings->rotateBy(-delta.x(), -delta.y());
		update();
	}
	else if (dragStatus == DRAG_PAN)
	{
		vpSettings->panBy(-0.05f * delta.x(), 0.05f * delta.y());
		update();
	}
	else if (dragStatus == DRAG_TOOL && currentTool)
	{
		ray_t ray = vpSettings->unproject(QVector3D(event->x(), height() - event->y(), 0.f));
		ToolEvent toolEvent(this, event, ray);
		currentTool->mouseMoved(toolEvent);

		if (scene->needsUpdate())
		{
			update();
		}
	}
	dragStart = event->pos();
}

void GlViewportWidget::wheelEvent(QWheelEvent *event)
{
	if (event->modifiers() & Qt::ControlModifier)
	{
		if (renderOptions.mode == RenderOptions::MODE_SLICE)
		{
			// TODO: some mice may report smaller deltas than 120 (15� * 8)
			renderOptions.level += event->angleDelta().y()/120;
			tesselationChanged = true;
			// adjust grid
			IVector3D low(-32, -32, -32), high(32, 32, 32);
			low[renderOptions.axis] = high[renderOptions.axis] = renderOptions.level;
			grid->setShape(renderOptions.axis, IBBox(low, high));
			update();
		}
	}
	else
	{
		QPoint delta = event->angleDelta();
		vpSettings->zoomBy(delta.y());
		update();
	}
}


void GlViewportWidget::on_activeToolChanged(EditTool *tool)
{
	if (tool != currentTool)
	{
		std::cout << "current tool changed\n";
		tool->initialize(scene);
		currentTool = tool;
	}
}

void GlViewportWidget::on_renderDataChanged()
{
	update();
}

void GlViewportWidget::on_layerSettingsChanged(int layerN, int change_flags)
{
	bool redraw = false;
	if (change_flags & VoxelLayer::VISIBILITY_CHANGED)
		redraw = true;
	if (layerN == scene->activeLayerN)
	{
		if (change_flags & VoxelLayer::BOUND_CHANGED)
		{
			boundCube->setShape(scene->layers[scene->activeLayerN]->bound);
			redraw = true;
		}
		if (change_flags & VoxelLayer::USE_BOUND_CHANGED)
			redraw = true;
	}
	if (redraw)
		update();
}
