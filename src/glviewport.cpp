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
	qDebug() << camPivot << "\n ray.from:" << ray.from << "\n ray.dir:" << ray. dir << "\n t_max:" << ray.t_max;
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
	float steps = dZ / 180.f;
	camDistance -= steps;
	if (camDistance < 1)
		camDistance = 1;
	else if (camDistance > 40)
		camDistance = 40;
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

GLRenderable *testObject;

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

	QOpenGLShaderProgram* voxelProgram = getShaderProgram(SHADER_VOXEL);
	voxelProgram->bind();
	// connect sRGB and material UBO
	block_index = glGetUniformBlockIndex(voxelProgram->programId(), "sRGB_LUT");
	glUniformBlockBinding(voxelProgram->programId(), block_index, 0);
	block_index = glGetUniformBlockIndex(voxelProgram->programId(), "materials");
	glUniformBlockBinding(voxelProgram->programId(), block_index, 1);
	// test: print UBO layout info
	//GLInfoLib::getUniformsInfo(voxelProgram->programId());

	//testobject
	LineGrid *grid = new LineGrid();
	grid->setSize(32);
	testObject = grid;

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
	// TODO: scene->render() without scene->update() loses dirty info, so probably force call internally
	if (scene->needsUpdate())
		scene->update();
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
	testObject->render(*this);

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
	return testObject;
}

void GlViewportWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		dragStatus = DRAG_TOOL;
		if (currentTool)
		{
			ray_t ray = vpSettings->unproject(QVector3D(event->x(), height() - event->y(), 0.f));
			ToolEvent toolEvent(event, ray);
			currentTool->mouseDown(toolEvent, *scene);

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
		ToolEvent toolEvent(event, ray);
		currentTool->mouseUp(toolEvent, *scene);
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
		ToolEvent toolEvent(event, ray);
		currentTool->mouseMoved(toolEvent, *scene);

		if (scene->needsUpdate())
		{
			update();
		}
	}
	dragStart = event->pos();
}

void GlViewportWidget::wheelEvent(QWheelEvent *event)
{
	QPoint delta = event->angleDelta();
	vpSettings->zoomBy(delta.y());
	update();
}


void GlViewportWidget::on_activeToolChanged(EditTool *tool)
{
	if (tool != currentTool)
	{
		std::cout << "current tool changed\n";
		currentTool = tool;
	}
}

void GlViewportWidget::on_renderDataChanged()
{
	update();
}
