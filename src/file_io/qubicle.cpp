/*
 * VoxelGem
 *
 *  Copyright 2018 by Lynx3d
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../voxelscene.h"
#include "../voxelaggregate.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <iostream>

union rgba_t
{
	uint32_t raw;
	struct
	{
		uint8_t r, g, b, a;
	};
	char bytes[4];
	rgba_t() {}
	rgba_t(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4):
		r(b1), g(b2), b(b3), a(b4) {}
};

struct mat_map_t
{
	rgba_t col;
	int property;
};

static const rgba_t col_mask(255, 255, 255, 0);
static const mat_map_t TypeMap[] =
{
	{ rgba_t(255, 255, 255, 255), Voxel::SOLID },
	{ rgba_t(128, 128, 128, 255), Voxel::GLASS },
	{ rgba_t( 64,  64,  64, 255), Voxel::TILED_GLASS },
	{ rgba_t(255,   0,   0, 255), Voxel::GLOWING_SOLID },
	{ rgba_t(255, 255,   0, 255), Voxel::GLOWING_GLASS }
};

static const mat_map_t SpecularMap[] =
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
		SceneOp(VoxelScene &lscene): scene(lscene) {}
		virtual void operator()(rgba_t data, int x, int y, int z) = 0;
		bool matches(rgba_t c1, rgba_t c2)
		{
			return ((c1.raw ^ c2.raw) & col_mask.raw) == 0;
		}
	protected:
		VoxelScene &scene;
};

class BaseColOp: public SceneOp
{
	public:
		BaseColOp(VoxelScene &lscene): SceneOp(lscene) {}
		void operator()(rgba_t data, int x, int y, int z) override
		{
			VoxelEntry voxel(data.raw, Voxel::VF_NON_EMPTY);
			int pos[3] = { x, y, z };
			scene.setVoxel(pos, voxel);
		}
};

class TypeMapOp: public SceneOp
{
	public:
		TypeMapOp(VoxelScene &lscene, const VoxelAggregate* bv): SceneOp(lscene), base(bv) {}
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
				int pos[3] = { x, y, z };
				const VoxelEntry *entry = base->getVoxel(pos);
				if (!entry || !(entry->flags & Voxel::VF_NON_EMPTY))
					return;
				VoxelEntry voxel(*entry);
				voxel.setMaterial(static_cast<Voxel::Material>(mat_type));
				scene.setVoxel(pos, voxel);
			}
		}
		const VoxelAggregate* base;
};

class SpecMapOp: public SceneOp
{
	public:
		SpecMapOp(VoxelScene &lscene, const VoxelAggregate* bv): SceneOp(lscene), base(bv) {}
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
				int pos[3] = { x, y, z };
				const VoxelEntry *entry = base->getVoxel(pos);
				if (!entry || !(entry->flags & Voxel::VF_NON_EMPTY))
					return;
				VoxelEntry voxel(*entry);
				voxel.setSpecular(static_cast<Voxel::Specular>(spec_type));
				scene.setVoxel(pos, voxel);
			}
		}
		const VoxelAggregate* base;
};

class AlphaMapOp: public SceneOp
{
	public:
		AlphaMapOp(VoxelScene &lscene, const VoxelAggregate* bv): SceneOp(lscene), base(bv) {}
		void operator()(rgba_t data, int x, int y, int z) override
		{
			if (data.r != data.b || data.r != data.g)
				return;

			int pos[3] = { x, y, z };
			const VoxelEntry *entry = base->getVoxel(pos);
			if (!entry || !(entry->flags & Voxel::VF_NON_EMPTY) || !entry->isTransparent())
				return;
			VoxelEntry voxel(*entry);
			voxel.col[3] = data.r;
			scene.setVoxel(pos, voxel);
		}
		const VoxelAggregate* base;
};

void parse_file(QDataStream &fstream, SceneOp &dataOp)
{
	// Qubicle files are little endian...
	fstream.setByteOrder(QDataStream::LittleEndian);
	const rgba_t CODEFLAG(2, 0, 0, 0);
	const rgba_t NEXTSLICEFLAG(6, 0, 0, 0);
	uint32_t version, colorFormat, zRight, compressed, visibilityMaskEncoded, numMatrices;
	fstream >> version;
	fstream >> colorFormat;
	fstream >> zRight;
	fstream >> compressed;
	fstream >> visibilityMaskEncoded;
	fstream >> numMatrices;
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

		if (compressed == 0) // uncompressd
		{
			for (uint32_t z = 0; z < sizeZ; z++)
				for (uint32_t y = 0; y < sizeY; y++)
					for (uint32_t x = 0; x < sizeX; x++)
			{
				fstream.readRawData(data.bytes, 4);
				if (data.a)
					dataOp(data, -posX - x, posY + y, (zRight ? posZ + z : - posZ - z));
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
					if (data.raw == NEXTSLICEFLAG.raw)
						break;
					runLength = 1;
					if (data.raw == CODEFLAG.raw)
					{
						fstream >> runLength;
						fstream.readRawData(data.bytes, 4);
					}
					for (uint32_t j = 0; j < runLength; ++j)
					{
						uint32_t x = index % sizeX;
						uint32_t y = index / sizeX;
						++index;
						if (data.a)
							dataOp(data, -posX - x, posY + y, (zRight ? posZ + z : - posZ - z));
					}
				}
			}
		}
	}
}

void qubicle_import(const QString &filename, VoxelScene &scene)
{
	QFileInfo file_info(filename);
	std::cout << "importing " << filename.toStdString() << std::endl;
	if (!file_info.isReadable())
		return;
	QFile file(file_info.filePath());
	if (!file.open(QIODevice::ReadOnly))
		return;
	QDataStream fstream(&file);
	BaseColOp colOp(scene);
	parse_file(fstream, colOp);
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
		TypeMapOp typeOp(scene, scene.getAggregate(0));
		parse_file(type_stream, typeOp);
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
		SpecMapOp specOp(scene, scene.getAggregate(0));
		parse_file(spec_stream, specOp);
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
		AlphaMapOp alphaOp(scene, scene.getAggregate(0));
		parse_file(alpha_stream, alphaOp);
	}
}
