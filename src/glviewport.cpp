/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glviewport.h"
//#include "voxelgrid.h"
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

const char* v_shader =
"#version 330\n"
"layout(location = 0) in vec3 v_position;\n"
"layout(location = 1) in uvec4 v_color;\n"
"out vec4 frag_color;\n"
"uniform mat4 mvp_mat;\n"
"layout (std140) uniform sRGB_LUT {\n"
"	float val[256];\n"
"};\n"
"void main() {\n"
"	gl_Position = mvp_mat * vec4(v_position, 1);\n"
"	frag_color = vec4(val[v_color.r], val[v_color.g], val[v_color.b], float(v_color.a)/255.0);\n"
//"	frag_color = v_color;\n"
"}\n";

const char* flat_shader =
"#version 330\n"
"in vec4 frag_color;\n"
"out vec4 final_color;\n"
"void main() {\n"
"	final_color = frag_color;\n"
"}";

const GlVertex_t testArray[] =
{
	{{ -5.f, 0.f, 0.f }, { 255, 0, 0, 255 }},
	{{ 5.f, 0.f, 0.f }, { 0, 255, 0, 255 }},
	{{ 0.f, 5.f, 0.f }, { 0, 0, 255, 255 }},
	// small triangle
	{{ -1.f, 0.f, -0.1f }, { 128, 0, 0, 255 }},
	{{ 1.f, 0.f, -0.1f }, { 0, 128, 0, 255 }},
	{{ 0.f, 1.f, -0.1f }, { 0, 0, 128, 255 }}
};

/* ========== GlViewportSettings ==============*/
/* QMatrix4x4().perspective(...) * QMatrix().lookAt(...);
   is equivalent to
   ( QMatrix4x4().perspective(...) ).lookAt(...);
*/

QMatrix4x4 ViewportSettings::getGlMatrix()
{
	return proj * view;
}

ray_t ViewportSettings::unproject(const QVector3D &pNear)
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
	ray.t_max = ray.dir.length();
	ray.dir /= ray.t_max;
	qDebug() << camPos << "\n ray.from:" << ray.from << "\n ray.dir:" << ray. dir << "\n t_max:" << ray.t_max;
	return ray;
}

void ViewportSettings::rotateBy(float dHead, float dPitch)
{
	heading += dHead;
	pitch += dPitch;
	while (heading < 0) heading += 360;
	while (heading > 360) heading -= 360;
	while (pitch < -180) pitch += 360;
	while (pitch > 180) pitch -= 360;
	updateViewMatrix();
}

void ViewportSettings::panBy(float dX, float dY)
{
	QVector4D offset = dX * view.row(0) + dY * view.row(1);
	camPos += QVector3D(offset);
	updateViewMatrix();
}

void ViewportSettings::updateViewport()
{
	proj.setToIdentity();
	proj.perspective(fov, (float)parent->width()/(float)parent->height(), 1, 100);
}

void ViewportSettings::updateViewMatrix()
{
	view.setToIdentity();
	view.rotate(QQuaternion::fromEulerAngles(pitch, heading, roll));
	//test
	view = view.transposed();
	view.translate(-camPos);
}

/* ========== GlViewportWidget ==============*/
// TODO: create proper shader library
QOpenGLShaderProgram *m_program, *m_voxel_program;
GLRenderable *testObject;
//VoxelGrid *testGrid;
//VoxelAggregate *testAggreg;
VoxelScene *testScene;
EditTool *testTool;

void GlViewportWidget::initializeGL()
{
	initializeOpenGLFunctions();
	glClearColor(0.2f, 0.3f, 0.35f, 1.0f);
	// debug
	QSurfaceFormat fmt = format();
	std::cout << "format depth buffer: " << fmt.depthBufferSize() << " GL Ver.: " << fmt.majorVersion() << "."
			<< fmt.minorVersion()
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
			<< " Color Space: " << fmt.colorSpace() /* Qt 5.10+ */
#endif
			<< std::endl;
	// init sRGB transfer function LUT (not exactly a gamma curve, but close)
	for (int i=0; i < 11; ++i) sRGB_LUT[i*4] = float(i)/(255.f * 12.92f);
	for (int i=11; i < 256; ++i) sRGB_LUT[i*4] = std::pow((float(i) + 0.055f)/(255.f * 1.055f), 2.4);

	// load shaders
	m_program = new QOpenGLShaderProgram(/*TODO: "self" as parent when stored in class*/);
	m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, v_shader);
	m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, flat_shader);
	m_program->link();
	m_program->bind();
	// test: print UBO layout info
	GLInfoLib::getUniformsInfo(m_program->programId());
	// connect sRGB UBO
	GLuint block_index = glGetUniformBlockIndex(m_program->programId(), "sRGB_LUT");
	glUniformBlockBinding(m_program->programId(), block_index, 0);

	m_voxel_program = new QOpenGLShaderProgram(/*TODO: "self" as parent when stored in class*/);
	m_voxel_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/voxel_fragment.glsl");
	m_voxel_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/voxel_vertex.glsl");
	m_voxel_program->link();
	m_voxel_program->bind();
	// connect sRGB UBO
	block_index = glGetUniformBlockIndex(m_voxel_program->programId(), "sRGB_LUT");
	glUniformBlockBinding(m_voxel_program->programId(), block_index, 0);

	// load LUT into ubo:
	glGenBuffers(1, &m_ubo_LUT);
	glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_LUT);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(sRGB_LUT), sRGB_LUT, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo_LUT);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	m_vertexSpec.create();
	m_vertexSpec.bind();
	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(testArray), testArray, GL_STATIC_DRAW);
	m_program->enableAttributeArray("v_position");
	//m_program->setAttributeBuffer(0, GL_FLOAT, ??, 3
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, pos));
	m_program->enableAttributeArray("v_color");
	//glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, col));
	glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(GlVertex_t), (const GLvoid*)offsetof(GlVertex_t, col));

	m_vertexSpec.release();
	m_program->release();

	//testobject
	testObject = new LineGrid();
	//testGrid = new VoxelGrid();
	//testAggreg = new VoxelAggregate();
	testScene = new VoxelScene();
	testTool = new PaintTool();
	//

	glEnable(GL_DEPTH_TEST);

	vpSettings = new ViewportSettings(this);
}
void GlViewportWidget::resizeGL(int w, int h)
{
	vpSettings->updateViewport();
}

void GlViewportWidget::paintGL()
{
	// TODO: multiply with devicePixelRatio() or the devicePixelRatioF() for Qt 5.6+
	glViewport(0, 0, width(), height());
	glEnable(GL_FRAMEBUFFER_SRGB); // no effect prior to Qt 5.10+ (no way to request sRGB buffers)

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_program->bind();
	m_vertexSpec.bind();
	/* disabled for test:
	QMatrix4x4 view, proj, final;
	view.lookAt(QVector3D(0.f, 0.f, -10.f), QVector3D(0.f, 0.f, 0.f), QVector3D(0.f, 1.f, 0.f));
	proj.perspective(45, (float)width()/float(height()), 1, 100);
	final = proj * view;
	-> test */
	QMatrix4x4 final = vpSettings->getGlMatrix();
	// end test
	m_program->setUniformValue("mvp_mat", final);
	glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_LUT);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo_LUT);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	m_vertexSpec.release();
	// test object
	testObject->render(*this);
	// test voxel grid object
	m_voxel_program->bind();
	m_voxel_program->setUniformValue("mvp_mat", final);
	//testGrid->render(*this);
	//testAggreg->render(*this);
	testScene->renderLayer->render(*this);
	m_program->release();
}

void GlViewportWidget::mousePressEvent(QMouseEvent *event)
{
	QMatrix4x4 view, proj, port, final;
	view.lookAt(QVector3D(0.f, 0.f, -10.f), QVector3D(0.f, 0.f, 0.f), QVector3D(0.f, 1.f, 0.f));
	proj.perspective(45, (float)width()/float(height()), 1, 100);
	final = proj * view;
	// yields same as above
	//proj.lookAt(QVector3D(0.f, 0.f, -10.f), QVector3D(0.f, 0.f, 0.f), QVector3D(0.f, 1.f, 0.f));
	//qDebug() << proj;
	port.viewport(0, 0, width(), height());
	final = port * final;
	QVector3D test(0.5f * width(), 0.5f * height(), 1.f);
	test = final.inverted() * test;
	//qDebug() << test;

	if (event->button() == Qt::LeftButton)
	{
		ray_t ray;
		ray = vpSettings->unproject(QVector3D(event->x(), height() - event->y(), 0.f));
		//int hitPos[3];
		SceneRayHit sceneHit;
		intersect_t hitInfo;
		bool didHit = testScene->renderLayer->rayIntersect(ray, sceneHit.voxelPos, hitInfo);//testAggreg->rayIntersect(ray, hitPos, hitInfo);
		//std::cout << "did hit:" << didHit << " voxel=(" << hitPos[0] << "," << hitPos[1] << "," << hitPos[2]
		//			<< ") hit axis:" << hitInfo.entryAxis << std::endl;
		if (didHit)
			sceneHit.flags |= SceneRayHit::HIT_VOXEL;
		else
		{
			didHit = testObject->rayIntersect(ray, sceneHit.voxelPos, hitInfo);
			if (didHit)
				sceneHit.flags |= SceneRayHit::HIT_LINEGRID;
		//	std::cout << "Grid hit:" << didHit << " voxel=(" << hitPos[0] << "," << hitPos[1] << "," << hitPos[2]
		//			<< ") hit axis:" << hitInfo.entryAxis << std::endl;
		}
		// voxel paint test
		if (didHit)
		{
			sceneHit.flags |= hitInfo.entryAxis;
			//hitPos[hitInfo.entryAxis & 3] += (hitInfo.entryAxis & intersect_t::AXIS_NEGATIVE) ? 1 : -1;
			VoxelEntry vox = { { 128, 128, 255, 255 }, VF_NON_EMPTY };
			//if (hitPos[0] >= 0 && hitPos[0] < 16 && hitPos[1] >= 0 && hitPos[1] < 16 && hitPos[2] >= 0 && hitPos[2] < 16)
			//	testGrid->setVoxel(hitPos[0], hitPos[1], hitPos[2], vox);
			//testAggreg->setVoxel(hitPos[0], hitPos[1], hitPos[2], vox);
			/*if (event->modifiers() & Qt::ShiftModifier)
				testScene->eraseVoxel(hitPos);
			else
			{
				hitPos[hitInfo.entryAxis & 3] += (hitInfo.entryAxis & intersect_t::AXIS_NEGATIVE) ? 1 : -1;
				testScene->setVoxel(hitPos, vox);
			}*/
			ToolEvent toolEvent(event, &sceneHit);
			testTool->mouseDown(toolEvent, *testScene);
			update();
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
	//vpSettings->headBy(22.5f);
	//update();
}

void GlViewportWidget::mouseReleaseEvent(QMouseEvent *event)
{
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
	dragStart = event->pos();
}
