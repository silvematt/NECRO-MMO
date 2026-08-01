#include "NDB.h"

#include <fstream>

#include "FileLogger.h"
#include "ConsoleLogger.h"

namespace NECRO
{
	const NDBValue* NDB::TryFind(const NDBRow& row, const std::string& colID) const
	{
		auto it = m_valuesMap.find(colID);
		if (it == m_valuesMap.end() || it->second >= row.m_values.size())
		{
			LOG_WARNING("NDB with ID: '{}'. Called TryFind on '{}', but it doesn't exist!", m_id, colID);
			return nullptr;
		}

		return &row.m_values[it->second];
	}

	const NDBValue* NDB::TryFind(uint32_t rowID, const std::string& colID) const
	{
		auto it = m_rows.find(rowID);
		if (it == m_rows.end())
		{
			LOG_WARNING("NDB with ID: '{}'. Called TryFind on RowID:'{}', but it doesn't exist!", m_id, rowID);
			return nullptr;
		}

		return TryFind(it->second, colID);
	}

	// Maps file markers to states
	static const std::unordered_map<std::string, NDBLoadState> ndbFileMarkers = 
	{
		{"DEFINITION_START", NDBLoadState::DEFINITION},
		{"DEFINITION_END",   NDBLoadState::NONE},
		{"STRUCTURE_START",  NDBLoadState::STRUCTURE},
		{"STRUCTURE_END",    NDBLoadState::NONE},
		{"ROWS_START",       NDBLoadState::ROWS},
		{"ROWS_END",         NDBLoadState::NONE},
	};


	bool NDB::LoadFromDisk(const std::string& path)
	{
		m_rows.clear();
		m_valuesMap.clear();
		m_id = "";

		std::ifstream ndbFile;

		ndbFile.open(path);

		if (!ndbFile.is_open())
		{
			LOG_ERROR("Could not load NDB at: {}.", path);
			return false;
		}

		// Intialize loading state
		NDBLoadState curState = NDBLoadState::NONE;

		// Data used while loading
		std::vector<std::string> structureTypes;	// all the data types - filled during Load and referenced while reading rows
		size_t curRowCount = 0;						// number of rows
		size_t structureCurIndex = 0;

		bool doneLoading = false;
		std::string line;
		while (!doneLoading)
		{
			std::getline(ndbFile, line);

			// Skip empty lines
			if (line.empty())
				continue;

			// Delete spaces
			line.erase(remove_if(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c); }), line.end());

			// Skip comments
			if (line[0] == '#')
				continue;

			// Look for markers and adjust state
			if (auto it = ndbFileMarkers.find(line); it != ndbFileMarkers.end()) 
			{
				curState = it->second;
				continue;
			}

			// Look for end
			if (line == "NDB_END")
			{
				doneLoading = true;
				break;
			}

			// Load
			switch (curState)
			{
				// Definitions are hard-coded and must be loaded in order
				case NDBLoadState::DEFINITION:
				{
					// ID
					m_id = line.substr(line.find(":") + 1);

					// If any other definition will be added, we'll std::getLine here and continue reading, like:
					// 
					// std::getLine(ndbFile, line)
					// NEXT_FIELD
					// m_nextField = line.substr(line.find(":") + 1);

					break;
				}

				case NDBLoadState::STRUCTURE:
				{
					size_t columnPos = line.find(':');
					std::string columnName = line.substr(0, columnPos);
					std::string columnType = line.substr(columnPos + 1);

					// columnName will be at position curIndex inside a row
					m_valuesMap[columnName] = structureCurIndex;
					structureCurIndex++;

					// We save the types of the data, so when we read a row we know what type it is going to be right away
					structureTypes.push_back(columnType);
					break;
				}

				case NDBLoadState::ROWS:
				{
					// Here we read whole row
					NDBRow row;

					// What we are currently reading (cursor on: 0,0,0,0,0)
					size_t currentColumnIndx = 0;
					uint32_t rowID = 0; // id of the row, written on the very first read (currentColumnIndx)

					// Get all the text until the next separator ','
					bool rowEnded = false;

					size_t startPos = 0;
					while (!rowEnded)
					{
						size_t commaPos = line.substr(startPos, std::string::npos).find(',');
						std::string curColumn = line.substr(startPos, commaPos);

						// Find last value of the row
						if (curColumn.find(';') != std::string::npos)
						{
							curColumn = curColumn.substr(0, curColumn.find(';'));
							rowEnded = true;
						}

						if (structureTypes[currentColumnIndx] == "int")
						{
							// Special Case: Read the ID of the row
							if (currentColumnIndx == 0)
							{
								rowID = std::stoi(curColumn);
							}

							// Any other int (including ID)
							row.m_values.push_back(NDBValue(std::stoi(curColumn)));
						}
						else if (structureTypes[currentColumnIndx] == "float")
						{
							row.m_values.push_back(NDBValue(std::stof(curColumn)));
						}
						else if (structureTypes[currentColumnIndx] == "bool")
						{
							if(curColumn == "true")
								row.m_values.push_back(NDBValue(true));
							else
								row.m_values.push_back(NDBValue(false));
						}
						else if (structureTypes[currentColumnIndx] == "string")
						{
							row.m_values.push_back(NDBValue(curColumn));
						}
						else
						{
							LOG_ERROR("NDB with ID: '{}' is ill formed! Structure contains a:'{}' which is not supported!", m_id, structureTypes[currentColumnIndx]);
							return false;
						}

						startPos += commaPos+1;
						currentColumnIndx++;
					}

					// Row has ended - insert it into the ndb
					auto it = m_rows.find(rowID);
					if (it == m_rows.end())
					{
						m_rows.insert({ rowID, row });
						curRowCount++;
					}
					else
						LOG_ERROR("NDB with ID: '{}'. Duplicated RowID:'{}'!", m_id, rowID);

					break;
				}

				default:
					continue;
			}
		}

		LOG_OK("'{}' successfully loaded! Loaded '{}' rows.", path, curRowCount);
		return true;
	}
}
