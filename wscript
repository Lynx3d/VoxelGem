#! /usr/bin/env python

top = '.'
out = 'build'

def options(opt):
	opt.load('compiler_cxx qt5')

def configure(conf):
	conf.load('compiler_cxx qt5')
	conf.env.append_value('CXXFLAGS', ['-g', '-Wall', '-std=c++11'])

def build(bld):
	# According to the Qt5 documentation:
	#   Qt classes in foo.h   -> declare foo.h as a header to be processed by moc
	#			    add the resulting moc_foo.cpp to the source files
	#   Qt classes in foo.cpp -> include foo.moc at the end of foo.cpp
	#
	sources = [ 'src/main.cpp',
				'src/edittool.cpp',
				'src/glviewport.cpp',
				'src/mainwindow.cpp',
				'src/palette.cpp',
				'src/renderobject.cpp',
				'src/util/shaderinfo.cpp',
				'src/voxelaggregate.cpp',
				'src/voxelgrid.cpp',
				'src/voxelscene.cpp',
				'resources.qrc',
				'mainwindow.ui']
	bld(
		features = 'qt5 cxx cxxprogram',
		use      = 'QT5CORE QT5GUI QT5SVG QT5WIDGETS',
		source   = sources, #'main.cpp glviewport.cpp renderobject.cpp voxelgrid.cpp resources.qrc mainwindow.ui',
		moc      = ['src/glviewport.h', 'src/mainwindow.h', 'src/palette.h'],
		target   = 'voxelGem',
		includes = '.',
		#lang     = bld.path.ant_glob('linguist/*.ts'),
		#langname = 'somefile', # include the .qm files from somefile.qrc
	)
