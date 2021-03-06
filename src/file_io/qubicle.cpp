/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../voxelscene.h"
#include "../sceneproxy.h"
#include "../voxelaggregate.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <iostream>

struct mat_map_t
{
	rgba_t col;
	int property;
};

const rgba_t CODEFLAG(2, 0, 0, 0);
const rgba_t NEXTSLICEFLAG(6, 0, 0, 0);
const rgba_t col_mask(255, 255, 255, 0);
const rgba_t refpoint_col(255, 0, 255, 255);
const mat_map_t TypeMap[] =
{
	{ rgba_t(255, 255, 255, 255), Voxel::SOLID },
	{ rgba_t(255,   0,   0, 255), Voxel::GLOWING_SOLID },
	{ rgba_t(128, 128, 128, 255), Voxel::GLASS },
	{ rgba_t( 64,  64,  64, 255), Voxel::TILED_GLASS },
	{ rgba_t(255, 255,   0, 255), Voxel::GLOWING_GLASS }
};

const mat_map_t SpecularMap[] =
{
	{ rgba_t(128,   0,   0, 255), Voxel::ROUGH },
	{ rgba_t(  0, 128,   0, 255), Voxel::METAL },
	{ rgba_t(  0,   0, 128, 255), Voxel::WATER },
	{ rgba_t(128, 128,   0, 255), Voxel::IRIDESCENT },
	{ rgba_t(  0, 128, 128, 255), Voxel::WAVE },
	{ rgba_t(128,   0, 128, 255), Voxel::WAXY }
};

class SceneOp
{
	public:
		virtual void operator()(rgba_t data, int x, int y, int z) = 0;
		virtual rgba_t operator()(int x, int y, int z) const { return rgba_t(0); } // TODO: make pure virtual
		void setAggregate(VoxelAggregate* agg) { aggregate = agg; }
		static bool matches(rgba_t c1, rgba_t c2)
		{
			return ((c1.raw ^ c2.raw) & col_mask.raw) == 0;
		}
	protected:
		VoxelAggregate* aggregate;
};

class BaseColOp: public SceneOp
{
	public:
		void operator()(rgba_t data, int x, int y, int z) override
		{
			VoxelEntry voxel(data.raw, Voxel::VF_NON_EMPTY);
			IVector3D pos(x, y, z);
			aggregate->setVoxel(pos, voxel);
		}
		rgba_t operator()(int x, int y, int z) const override
		{
			IVector3D pos(x, y, z);
			const VoxelEntry *entry = aggregate->getVoxel(pos);
			if (entry && entry->flags & Voxel::VF_NON_EMPTY)
			{
				rgba_t data(entry->col.raw);
				data.a = 255;
				return data;
			}
			return rgba_t(0);
		}
};

class TypeMapOp: public SceneOp
{
	public:
		void operator()(rgba_t data, int x, int y, int z) override
		{
			int mat_type = -1;
			for (int i = 0; i < 5; ++i)
				if (matches(data, TypeMap[i].col))
				{
					mat_type = TypeMap[i].property;
					break;
				}
			if (mat_type != -1)
			{
				IVector3D pos(x, y, z);
				const VoxelEntry *entry = aggregate->getVoxel(pos);
				if (!entry || !(entry->flags & Voxel::VF_NON_EMPTY))
					return;
				VoxelEntry voxel(*entry);
				voxel.setMaterial(static_cast<Voxel::Material>(mat_type));
				aggregate->setVoxel(pos, voxel);
			}
		}
		rgba_t operator()(int x, int y, int z) const override
		{
			IVector3D pos(x, y, z);
			const VoxelEntry *entry = aggregate->getVoxel(pos);
			if (entry && entry->flags & Voxel::VF_NON_EMPTY)
			{
				int material = entry->getMaterial();
				if (matches(entry->col, refpoint_col))
					return refpoint_col;
				return TypeMap[material].col;
			}
			return rgba_t(0);
		}
};

class SpecMapOp: public SceneOp
{
	public:
		void operator()(rgba_t data, int x, int y, int z) override
		{
			int spec_type = -1;
			for (int i = 0; i < 6; ++i)
				if (matches(data, SpecularMap[i].col))
				{
					spec_type = SpecularMap[i].property;
					break;
				}
			if (spec_type != -1)
			{
				IVector3D pos(x, y, z);
				const VoxelEntry *entry = aggregate->getVoxel(pos);
				if (!entry || !(entry->flags & Voxel::VF_NON_EMPTY))
					return;
				VoxelEntry voxel(*entry);
				voxel.setSpecular(static_cast<Voxel::Specular>(spec_type));
				aggregate->setVoxel(pos, voxel);
			}
		}
		rgba_t operator()(int x, int y, int z) const override
		{
			IVector3D pos(x, y, z);
			const VoxelEntry *entry = aggregate->getVoxel(pos);
			if (entry && entry->flags & Voxel::VF_NON_EMPTY)
			{
				int specular = entry->getSpecular();
				if (matches(entry->col, refpoint_col))
					return refpoint_col;
				return SpecularMap[specular].col;
			}
			return rgba_t(0);
		}
};

class AlphaMapOp: public SceneOp
{
	public:
		void operator()(rgba_t data, int x, int y, int z) override
		{
			if (data.r != data.b || data.r != data.g)
				return;

			IVector3D pos(x, y, z);
			const VoxelEntry *entry = aggregate->getVoxel(pos);
			if (!entry || !(entry->flags & Voxel::VF_NON_EMPTY) /* || !entry->isTransparent() */)
				return;
			VoxelEntry voxel(*entry);
			voxel.col.a = data.r;
			aggregate->setVoxel(pos, voxel);
		}
		rgba_t operator()(int x, int y, int z) const override
		{
			IVector3D pos(x, y, z);
			const VoxelEntry *entry = aggregate->getVoxel(pos);
			if (entry && entry->flags & Voxel::VF_NON_EMPTY)
			{
				if (matches(entry->col, refpoint_col))
					return refpoint_col;
				uint8_t alpha = (entry->col.a == 255) ? 255 : (entry->col.a & 0xE0) + 0x10;
				return rgba_t(alpha, alpha, alpha, 255);
			}
			return rgba_t(0);
		}
};

bool parse_file(QDataStream &fstream, SceneOp &dataOp, std::vector<VoxelLayer*> &layers)
{
	bool create = (layers.size() == 0);
	// Qubicle files are little endian...
	fstream.setByteOrder(QDataStream::LittleEndian);
	uint32_t version, colorFormat, zRight, compressed, visibilityMaskEncoded, numMatrices;
	fstream >> version;
	fstream >> colorFormat;
	fstream >> zRight;
	fstream >> compressed;
	fstream >> visibilityMaskEncoded;
	fstream >> numMatrices;
	if (fstream.status() != QDataStream::Ok)
		return false;
	rgba_t data;
	for (uint32_t i=0; i < numMatrices; ++i)
	{
		uint8_t nameLength;
		char nameBuff[257];
		fstream >> nameLength;
		fstream.readRawData(nameBuff, nameLength);
		nameBuff[nameLength] = '\0';
		std::cout << "loading layer '" << nameBuff << "' " << i+1 << "/" << numMatrices << std::endl;

		uint32_t sizeX, sizeY, sizeZ;
		fstream >> sizeX >>  sizeY >> sizeZ;

		uint32_t  posX, posY, posZ;
		fstream >> posX >> posY >> posZ;

		if (fstream.status() != QDataStream::Ok)
			return false;

		if (create)
		{
			VoxelLayer* newLayer = new VoxelLayer;
			newLayer->aggregate = new VoxelAggregate();
			newLayer->bound.pMin = IVector3D(posX, posY, zRight ? -posZ - sizeZ + 1 : posZ);
			newLayer->bound.pMax = IVector3D(posX + sizeX, posY + sizeY, zRight ? -posZ + 1 : posZ + sizeZ);
			newLayer->useBound = true;
			newLayer->name = std::string(nameBuff);
			layers.push_back(newLayer);
			dataOp.setAggregate(newLayer->aggregate);
		}
		else
		{
			dataOp.setAggregate(layers[i]->aggregate);
		}

		if (compressed == 0) // uncompressd
		{
			for (uint32_t z = 0; z < sizeZ; z++)
				for (uint32_t y = 0; y < sizeY; y++)
					for (uint32_t x = 0; x < sizeX; x++)
			{
				fstream.readRawData(data.bytes, 4);
				if (fstream.status() != QDataStream::Ok)
					return false;
				if (data.a)
					dataOp(data, posX + x, posY + y, (zRight ? -posZ - z : posZ + z));
			}
		}
		else // RLE compressed
		{
			std::cout << "RLE compressed\n";
			uint32_t runLength, index;
			for (uint32_t z = 0; z < sizeZ; ++z)
			{
				index = 0;
				while (true)
				{
					fstream.readRawData(data.bytes, 4);
					if (fstream.status() != QDataStream::Ok)
							return false;
					if (data.raw == NEXTSLICEFLAG.raw)
						break;
					runLength = 1;
					if (data.raw == CODEFLAG.raw)
					{
						fstream >> runLength;
						fstream.readRawData(data.bytes, 4);
						if (fstream.status() != QDataStream::Ok)
							return false;
					}
					for (uint32_t j = 0; j < runLength; ++j)
					{
						uint32_t x = index % sizeX;
						uint32_t y = index / sizeX;
						++index;
						if (data.a)
							dataOp(data, posX + x, posY + y, (zRight ? -posZ - z : posZ + z));
					}
				}
			}
		}
	}
	return fstream.status() == QDataStream::Ok;
}

void qubicle_import(const QString &filename, SceneProxy *sceneP)
{
	QFileInfo file_info(filename);
	std::cout << "importing " << filename.toStdString() << std::endl;
	if (!file_info.isReadable())
		return;
	QFile file(file_info.filePath());
	if (!file.open(QIODevice::ReadOnly))
		return;
	QDataStream fstream(&file);
	BaseColOp colOp;
	std::vector<VoxelLayer*> fileLayers;
	parse_file(fstream, colOp, fileLayers);
	if (fileLayers.size() == 0)
	{
		std::cout << "error: no layer read from file.\n";
		return;
	}
	// Trove specific:
	// check if we have files with _t, _a and _s suffix for material properties
	QString base = file_info.completeBaseName();
	QString suffix = file_info.suffix();
	//== Type Map ==//
	QFileInfo typemap_info(file_info.dir(), base + "_t." + suffix);
	std::cout << "looking for " << typemap_info.filePath().toStdString() << std::endl;
	if (typemap_info.isReadable())
	{
		std::cout << "reading type map '" << typemap_info.filePath().toStdString() << "'\n";
		QFile type_file(typemap_info.filePath());
		if (!type_file.open(QIODevice::ReadOnly))
			return;
		QDataStream type_stream(&type_file);
		TypeMapOp typeOp;
		parse_file(type_stream, typeOp, fileLayers);
	}
	//== Specular Map ==//
	QFileInfo specmap_info(file_info.dir(), base + "_s." + suffix);
	std::cout << "looking for " << specmap_info.filePath().toStdString() << std::endl;
	if (specmap_info.isReadable())
	{
		std::cout << "reading type map '" << specmap_info.filePath().toStdString() << "'\n";
		QFile spec_file(specmap_info.filePath());
		if (!spec_file.open(QIODevice::ReadOnly))
			return;
		QDataStream spec_stream(&spec_file);
		SpecMapOp specOp;
		parse_file(spec_stream, specOp, fileLayers);
	}
	//== Alpha Map ==//
	QFileInfo alphamap_info(file_info.dir(), base + "_a." + suffix);
	std::cout << "looking for " << alphamap_info.filePath().toStdString() << std::endl;
	if (alphamap_info.isReadable())
	{
		std::cout << "reading alpha map '" << alphamap_info.filePath().toStdString() << "'\n";
		QFile alpha_file(alphamap_info.filePath());
		if (!alpha_file.open(QIODevice::ReadOnly))
			return;
		QDataStream alpha_stream(&alpha_file);
		AlphaMapOp alphaOp;
		parse_file(alpha_stream, alphaOp, fileLayers);
	}
	for (auto &layer: fileLayers)
		sceneP->insertLayer(layer);
}

void write_file_header(QDataStream &fstream, uint32_t numLayers, bool compressed)
{
	fstream << (uint32_t)0x00000101; // Version 1.1.0.0 in little endian
	fstream << (uint32_t)0; // RGBA format
	fstream << (uint32_t)1; // right-handed
	fstream << (uint32_t)compressed;
	fstream << (uint32_t)0; // no visibility mask encoding
	fstream << numLayers; // currently only support 1 matrix
}

void writeRun(QDataStream &fstream, rgba_t col, uint32_t count)
{
	// count == 0 happens at new slice, need to skip
	if (count > 0 && count <= 3)
		for (uint32_t i = 0; i < count; ++i)
			fstream << col.raw;
	else if (count > 3)
	{
		fstream << CODEFLAG.raw;
		fstream << count;
		fstream << col.raw;
	}
}

void write_layer(QDataStream &fstream, const std::string &name, IBBox bound, SceneOp &dataOp, bool compressed)
{
	// name length
	uint8_t nameLen = std::min(255, (int)name.length());
	fstream << nameLen;
	fstream.writeRawData(name.data(), nameLen);
	// matrix size
	fstream << bound.pMax[0] - bound.pMin[0];
	fstream << bound.pMax[1] - bound.pMin[1];
	fstream << bound.pMax[2] - bound.pMin[2];
	// matrix position; z gets inverted here => upper bound becomes lower pos
	fstream << bound.pMin[0] << bound.pMin[1] << -bound.pMax[2] + 1;
	if (compressed)
	{
		// voxels; z gets inverted here
		for (int z =  bound.pMax[2] - 1; z >= bound.pMin[2]; --z)
		{
			rgba_t lastData(0);
			uint32_t count = 0;

			for (int y = bound.pMin[1]; y < bound.pMax[1]; ++y)
				for (int x = bound.pMin[0]; x < bound.pMax[0]; ++x)
			{
				rgba_t data = dataOp(x, y, z);
				if (data != lastData)
				{
					writeRun(fstream, lastData, count);
					count = 0;
					lastData = data;
				}
				++count;
			}
			writeRun(fstream, lastData, count);
			count = 0;
			fstream << NEXTSLICEFLAG.raw;
		}
	}
	else
	{
		// voxels; z gets inverted here
		for (int z =  bound.pMax[2] - 1; z >= bound.pMin[2]; --z)
			for (int y = bound.pMin[1]; y < bound.pMax[1]; ++y)
				for (int x = bound.pMin[0]; x < bound.pMax[0]; ++x)
		{
			fstream << dataOp(x, y, z).raw;
		}
	}
}

void qubicle_export(const QString &filename, SceneProxy *sceneP, bool trove_maps)
{
	bool compressed = true;
	QFileInfo file_info(filename);
	std::cout << "exporting " << filename.toStdString() << std::endl;
	QFile file(file_info.filePath()), type_file, spec_file, alpha_file;
	if (!file.open(QIODevice::WriteOnly))
		return;
	QDataStream fstream(&file), type_stream, spec_stream, alpha_stream;
	// Qubicle files are little endian...
	fstream.setByteOrder(QDataStream::LittleEndian);
	BaseColOp colOp;
	TypeMapOp typeOp;
	SpecMapOp specOp;
	AlphaMapOp alphaOp;
	int layerCount = sceneP->layerCount();
	write_file_header(fstream, layerCount, compressed);

	if (trove_maps)
	{
		QString base = file_info.completeBaseName();
		QString suffix = file_info.suffix();
		//== Type Map ==//
		QFileInfo typemap_info(file_info.dir(), base + "_t." + suffix);
		std::cout << "exporting " << typemap_info.filePath().toStdString() << std::endl;
		type_file.setFileName(typemap_info.filePath());
		if (!type_file.open(QIODevice::WriteOnly))
			return;
		type_stream.setDevice(&type_file);
		type_stream.setByteOrder(QDataStream::LittleEndian);
		write_file_header(type_stream, layerCount, compressed);
		//== Specular Map ==//
		QFileInfo specmap_info(file_info.dir(), base + "_s." + suffix);
		std::cout << "exporting " << specmap_info.filePath().toStdString() << std::endl;
		spec_file.setFileName(specmap_info.filePath());
		if (!spec_file.open(QIODevice::WriteOnly))
			return;
		spec_stream.setDevice(&spec_file);
		spec_stream.setByteOrder(QDataStream::LittleEndian);
		write_file_header(spec_stream, layerCount, compressed);
		//== Alpha Map ==//
		QFileInfo alphamap_info(file_info.dir(), base + "_a." + suffix);
		std::cout << "exporting " << alphamap_info.filePath().toStdString() << std::endl;
		alpha_file.setFileName(alphamap_info.filePath());
		if (!alpha_file.open(QIODevice::WriteOnly))
			return;
		alpha_stream.setDevice(&alpha_file);
		alpha_stream.setByteOrder(QDataStream::LittleEndian);
		write_file_header(alpha_stream, layerCount, compressed);
	}

	for (int i = 0; i < layerCount; ++i)
	{
		const VoxelLayer *layer = sceneP->getLayer(i);
		colOp.setAggregate(layer->aggregate);
		IBBox sceneBound(IVector3D(0,0,0), IVector3D(0,0,0)); // TODO: proper constructor
		if (layer->useBound)
			sceneBound = layer->bound;
		else
			layer->aggregate->getBound(sceneBound);
		write_layer(fstream, layer->name, sceneBound, colOp, compressed);
		if (trove_maps)
		{
			typeOp.setAggregate(layer->aggregate);
			write_layer(type_stream, layer->name, sceneBound, typeOp, compressed);
			specOp.setAggregate(layer->aggregate);
			write_layer(spec_stream, layer->name, sceneBound, specOp, compressed);
			alphaOp.setAggregate(layer->aggregate);
			write_layer(alpha_stream, layer->name, sceneBound, alphaOp, compressed);
		}
	}
}

void qubicle_export_layer(const QString &filename, SceneProxy *sceneP, bool trove_maps)
{
	bool compressed = true;
	QFileInfo file_info(filename);
	if (!file_info.isWritable())
	{
		std::cout << filename.toStdString() << " not writable." << std::endl;
		//return; // seems false when file doesn't exist yet
	}
	std::cout << "exporting " << filename.toStdString() << std::endl;
	QFile file(file_info.filePath());
	if (!file.open(QIODevice::WriteOnly))
		return;
	QDataStream fstream(&file);
	// Qubicle files are little endian...
	fstream.setByteOrder(QDataStream::LittleEndian);
	const VoxelLayer *layer = sceneP->getLayer(sceneP->activeLayer());
	BaseColOp colOp;
	colOp.setAggregate(layer->aggregate);
	IBBox sceneBound(IVector3D(0,0,0), IVector3D(0,0,0)); // TODO: proper constructor
	if (layer->useBound)
		sceneBound = layer->bound;
	else
		layer->aggregate->getBound(sceneBound);
	write_file_header(fstream, 1, compressed);
	write_layer(fstream, layer->name, sceneBound, colOp, compressed);
	if (trove_maps)
	{
		QString base = file_info.completeBaseName();
		QString suffix = file_info.suffix();
		//== Type Map ==//
		QFileInfo typemap_info(file_info.dir(), base + "_t." + suffix);
		std::cout << "exporting " << typemap_info.filePath().toStdString() << std::endl;
		QFile type_file(typemap_info.filePath());
		if (!type_file.open(QIODevice::WriteOnly))
			return;
		QDataStream type_stream(&type_file);
		type_stream.setByteOrder(QDataStream::LittleEndian);
		TypeMapOp typeOp;
		typeOp.setAggregate(layer->aggregate);
		write_file_header(type_stream, 1, compressed);
		write_layer(type_stream, layer->name, sceneBound, typeOp, compressed);
		//== Specular Map ==//
		QFileInfo specmap_info(file_info.dir(), base + "_s." + suffix);
		std::cout << "exporting " << specmap_info.filePath().toStdString() << std::endl;
		QFile spec_file(specmap_info.filePath());
		if (!spec_file.open(QIODevice::WriteOnly))
			return;
		QDataStream spec_stream(&spec_file);
		spec_stream.setByteOrder(QDataStream::LittleEndian);
		SpecMapOp specOp;
		specOp.setAggregate(layer->aggregate);
		write_file_header(spec_stream, 1, compressed);
		write_layer(spec_stream, layer->name, sceneBound, specOp, compressed);
		//== Alpha Map ==//
		QFileInfo alphamap_info(file_info.dir(), base + "_a." + suffix);
		std::cout << "exporting " << alphamap_info.filePath().toStdString() << std::endl;
		QFile alpha_file(alphamap_info.filePath());
		if (!alpha_file.open(QIODevice::WriteOnly))
			return;
		QDataStream alpha_stream(&alpha_file);
		alpha_stream.setByteOrder(QDataStream::LittleEndian);
		AlphaMapOp alphaOp;
		alphaOp.setAggregate(layer->aggregate);
		write_file_header(alpha_stream, 1, compressed);
		write_layer(alpha_stream, layer->name, sceneBound, alphaOp, compressed);
	}
}
