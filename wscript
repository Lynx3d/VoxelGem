#! /usr/bin/env python

top = '.'
out = 'build'

def options(opt):
	opt.load('compiler_cxx qt5')
	opt.add_option('--debug_gl', default=False, action='store_true', help='enable OpenGL debug context')

def configure(conf):
	conf.load('compiler_cxx qt5')
	conf.env.append_value('CXXFLAGS', ['-g', '-Wall', '-std=c++11'])
	if conf.options.debug_gl:
		conf.define('DEBUG_GL', 1)

def build(bld):
	# According to the Qt5 documentation:
	#   Qt classes in foo.h   -> declare foo.h as a header to be processed by moc
	#			    add the resulting moc_foo.cpp to the source files
	#   Qt classes in foo.cpp -> include foo.moc at the end of foo.cpp
	#
	moc_src = [ 'src/glviewport.h',
				'src/mainwindow.h',
				'src/palette.h',
				'src/sceneproxy.h',
				'src/gui/layereditor.h' ]

	sources = [ 'src/main.cpp',
				'src/edittool.cpp',
				'src/glviewport.cpp',
				'src/mainwindow.cpp',
				'src/palette.cpp',
				'src/renderobject.cpp',
				'src/sceneproxy.cpp',
				'src/shading.cpp',
				'src/util/shaderinfo.cpp',
				'src/voxelaggregate.cpp',
				'src/voxelgrid.cpp',
				'src/voxelscene.cpp',
				'src/file_io/qubicle.cpp',
				'src/gui/layereditor.cpp',
				'src/gui/layereditor.ui',
				'src/tools/draw.cpp',
				'resources.qrc',
				'mainwindow.ui']
	bld(
		features = 'qt5 cxx cxxprogram',
		use      = 'QT5CORE QT5GUI QT5SVG QT5WIDGETS QT5OPENGL',
		source   = sources,
		moc      = moc_src,
		target   = 'voxelGem',
		includes = ['.', './src'],
		#lang     = bld.path.ant_glob('linguist/*.ts'),
		#langname = 'somefile', # include the .qm files from somefile.qrc
	)
