#pragma once
#include <iostream>
#include <string>
#include "h2bParser.h"

struct FileData
{
	std::string type;
	std::string name;
	GW::MATH::GMATRIXF pos;
};

class FileIO
{
public:
	FileIO();
	~FileIO();
	std::vector<FileData> gameLevelObjects;
	std::vector<H2B::Parser> meshAndMaterialData;
	//std::vector<std::string> nameList;
	H2B::Parser parsedData;
	string h2BLocationFolder;

	void ReadFile(std::string _fileName)
	{
		std::ifstream levelFile(_fileName);
		if (levelFile)
		{
			std::string line;
			std::getline(levelFile, line);
			std::getline(levelFile, line);
			while (!levelFile.eof())
			{
				FileData fd;
				fd.type = line;
				std::getline(levelFile, line);
				fd.name = line;
				std::getline(levelFile, line, '(');
				std::getline(levelFile, line, ',');
				fd.pos.row1.x = std::stof(line);
				std::getline(levelFile, line, ',');
				fd.pos.row1.y = std::stof(line);
				std::getline(levelFile, line, ',');
				fd.pos.row1.z = std::stof(line);
				std::getline(levelFile, line);
				fd.pos.row1.w = std::stof(line);
				std::getline(levelFile, line, '(');
				std::getline(levelFile, line, ',');
				fd.pos.row2.x = std::stof(line);
				std::getline(levelFile, line, ',');
				fd.pos.row2.y = std::stof(line);
				std::getline(levelFile, line, ',');
				fd.pos.row2.z = std::stof(line);
				std::getline(levelFile, line);
				fd.pos.row2.w = std::stof(line);
				std::getline(levelFile, line, '(');
				std::getline(levelFile, line, ',');
				fd.pos.row3.x = std::stof(line);
				std::getline(levelFile, line, ',');
				fd.pos.row3.y = std::stof(line);
				std::getline(levelFile, line, ',');
				fd.pos.row3.z = std::stof(line);
				std::getline(levelFile, line);
				fd.pos.row3.w = std::stof(line);
				std::getline(levelFile, line, '(');
				std::getline(levelFile, line, ',');
				fd.pos.row4.x = std::stof(line);
				std::getline(levelFile, line, ',');
				fd.pos.row4.y = std::stof(line);
				std::getline(levelFile, line, ',');
				fd.pos.row4.z = std::stof(line);
				std::getline(levelFile, line);
				fd.pos.row4.w = std::stof(line);
				gameLevelObjects.push_back(fd);
				std::getline(levelFile, line);
				if (line == "")
				{
					std::getline(levelFile, line);
				}
				else if (levelFile.eof())
				{
					break;
				}
			}
		}
		else std::cout << "Unable to open file";
		levelFile.close();
	}
	void ReadH2B()
	{
		for (size_t i = 0; i < gameLevelObjects.size(); i++)
		{
			if (gameLevelObjects[i].type != "MESH")
			{
				continue;
			}
			char* h2bFileName;
			int count = 0;
			std::string objectName(gameLevelObjects[i].name);
			for (size_t j = 0; j < objectName.length(); j++)
			{
				if (objectName[j] == '.')
				{
					int startSize = objectName.length();
					for (size_t k = 0; k < startSize - count; k++)
					{
						objectName.pop_back();
					}
					break;
				}
				count++;
			}
			/*bool repeat = false;
			for (size_t z = 0; z < nameList.size(); z++)
			{
				if (nameList[z].compare(objectName) == 0)
				{
					repeat = true;
					gameLevelObjects[i].baseModelIndex = z;
					break;
				}
			}*/
			/*if (!repeat)
			{*/
			//nameList.push_back(objectName);
			objectName.insert(0, "../" + h2BLocationFolder + "/");
			objectName.append(".h2b");
			h2bFileName = &objectName[0];
			parsedData.Parse(h2bFileName);
			meshAndMaterialData.push_back(parsedData);
			//}
		}
	}
};

FileIO::FileIO()
{
}

FileIO::~FileIO()
{
	//gameLevelObjects.clear();
	//meshAndMaterialData.clear();
}