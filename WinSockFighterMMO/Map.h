#pragma once

namespace azely {

//-----------------------------------------------------------------
// 화면 이동 범위.
//-----------------------------------------------------------------
	enum Map
	{
		X_MIN = 0,
		X_MAX = 6400,
		Y_MIN = 0,
		Y_MAX = 6400
	};

	struct MapSector {
		int x;
		int y;
	};

	struct MapSectorGroup {
		MapSectorGroup() {
			sectorCount = 0;
		}
		int sectorCount;
		MapSector sectors[9];
	};

	constexpr int	SECTOR_SCALE = 3;
	constexpr int	SECTOR_WIDTH = X_MAX / 100 * SECTOR_SCALE;
	constexpr int	SECTOR_HEIGHT = Y_MAX / 100 * SECTOR_SCALE;
	constexpr int	SECTOR_INDEX_SIZE_X = X_MAX / SECTOR_WIDTH;
	constexpr int	SECTOR_INDEX_SIZE_Y = Y_MAX / SECTOR_HEIGHT;

	inline INT32 GetMapSectorIndexX(int posX) {
		return min(posX / SECTOR_WIDTH, SECTOR_INDEX_SIZE_X - 1);
	}

	inline INT32 GetMapSectorIndexY(int posY) {
		return min(posY / SECTOR_HEIGHT, SECTOR_INDEX_SIZE_Y - 1);
	}

}