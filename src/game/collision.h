/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

#include <engine/shared/protocol.h>

#include <map>
#include <vector>

class CTile;
class CLayers;
class CTeleTile;
class CSpeedupTile;
class CSwitchTile;
class CTuneTile;
class CDoorTile;

enum
{
	CANTMOVE_LEFT = 1 << 0,
	CANTMOVE_RIGHT = 1 << 1,
	CANTMOVE_UP = 1 << 2,
	CANTMOVE_DOWN = 1 << 3,
};

wvec2 ClampVel(int MoveRestriction, wvec2 Vel);

typedef bool (*CALLBACK_SWITCHACTIVE)(unsigned char Number, void *pUser);
struct CAntibotMapData;

class CCollision
{
public:
	CCollision();
	~CCollision();

	void Init(CLayers *pLayers);
	void Unload();
	void FillAntibot(CAntibotMapData *pMapData) const;

	void SetHoTileTeleEnabled(bool Enabled) { m_HoTileTeleEnabled = Enabled; }

	bool CheckPoint(wcoord x, wcoord y) const { return IsSolid(round_to_int(x), round_to_int(y)); }
	bool CheckPoint(wvec2 Pos) const { return CheckPoint(Pos.x, Pos.y); }
	bool CheckPoint(float x, float y) const { return CheckPoint(wcoord(x), wcoord(y)); }
	bool CheckPoint(vec2 Pos) const { return CheckPoint(wcoord(Pos.x), wcoord(Pos.y)); }
	int GetCollisionAt(wcoord x, wcoord y) const { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetCollisionAt(float x, float y) const { return GetCollisionAt(wcoord(x), wcoord(y)); }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	int IntersectLine(wvec2 Pos0, wvec2 Pos1, wvec2 *pOutCollision, wvec2 *pOutBeforeCollision) const;
	int IntersectLineTeleWeapon(wvec2 Pos0, wvec2 Pos1, wvec2 *pOutCollision, wvec2 *pOutBeforeCollision, int *pTeleNr = nullptr) const;
	int IntersectLineTeleHook(wvec2 Pos0, wvec2 Pos1, wvec2 *pOutCollision, wvec2 *pOutBeforeCollision, int *pTeleNr = nullptr) const;
	void MovePoint(wvec2 *pInoutPos, wvec2 *pInoutVel, float Elasticity, int *pBounces) const;
	void MoveBox(wvec2 *pInoutPos, wvec2 *pInoutVel, wvec2 Size, vec2 Elasticity, bool *pGrounded = nullptr) const;
	bool TestBox(wvec2 Pos, wvec2 Size) const;
	bool IsOnGround(wvec2 Pos, float Size) const;
	bool IsOnGround(vec2 Pos, float Size) const { return IsOnGround(wvec2(Pos), Size); }

	// Float vec2 adapters (client rendering / particles)
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const;
	int IntersectLineTeleWeapon(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, int *pTeleNr = nullptr) const;
	int IntersectLineTeleHook(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, int *pTeleNr = nullptr) const;
	int IntersectNoLaser(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const;
	int IntersectNoLaserNoWalls(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const;
	int IntersectAir(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const;
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces) const;
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, vec2 Elasticity, bool *pGrounded = nullptr) const;
	bool TestBox(vec2 Pos, vec2 Size) const { return TestBox(wvec2(Pos), wvec2(Size)); }
	void GetSpeedup(int Index, vec2 *pDir, int *pForce, int *pMaxSpeed, int *pType) const;

	// DDRace
	void SetCollisionAt(float x, float y, int Index);
	void SetDoorCollisionAt(float x, float y, unsigned char Type, unsigned char Flags, unsigned char Number);
	void GetDoorTile(int Index, CDoorTile *pDoorTile) const;
	int GetFrontCollisionAt(wcoord x, wcoord y) const { return GetFrontTile(round_to_int(x), round_to_int(y)); }
	int IntersectNoLaser(wvec2 Pos0, wvec2 Pos1, wvec2 *pOutCollision, wvec2 *pOutBeforeCollision) const;
	int IntersectNoLaserNoWalls(wvec2 Pos0, wvec2 Pos1, wvec2 *pOutCollision, wvec2 *pOutBeforeCollision) const;
	int IntersectAir(wvec2 Pos0, wvec2 Pos1, wvec2 *pOutCollision, wvec2 *pOutBeforeCollision) const;
	int GetIndex(int x, int y) const;
	int GetIndex(wvec2 PrevPos, wvec2 Pos) const;
	int GetFrontIndex(int x, int y) const;

	int GetMoveRestrictions(CALLBACK_SWITCHACTIVE pfnSwitchActive, void *pUser, wvec2 Pos, float Distance = 18.0f, int OverrideCenterTileIndex = -1) const;
	int GetMoveRestrictions(wvec2 Pos, float Distance = 18.0f) const
	{
		return GetMoveRestrictions(nullptr, nullptr, Pos, Distance);
	}

	int GetTile(int x, int y) const;
	int GetFrontTile(int x, int y) const;
	int Entity(int x, int y, int Layer) const;
	int GetPureMapIndex(wcoord x, wcoord y) const;
	int GetPureMapIndex(wvec2 Pos) const { return GetPureMapIndex(Pos.x, Pos.y); }
	std::vector<int> GetMapIndices(wvec2 PrevPos, wvec2 Pos, unsigned MaxIndices = 0) const;
	int GetMapIndex(wvec2 Pos) const;
	int GetMapIndex(vec2 Pos) const { return GetMapIndex(wvec2(Pos)); }
	int GetPureMapIndex(vec2 Pos) const { return GetPureMapIndex(wcoord(Pos.x), wcoord(Pos.y)); }
	bool TileExists(int Index) const;
	bool TileExistsNext(int Index) const;
	wvec2 GetPos(int Index) const;
	int GetTileIndex(int Index) const;
	int GetFrontTileIndex(int Index) const;
	int GetTileFlags(int Index) const;
	int GetFrontTileFlags(int Index) const;
	int IsTeleport(int Index) const;
	int IsEvilTeleport(int Index) const;
	bool IsCheckTeleport(int Index) const;
	bool IsCheckEvilTeleport(int Index) const;
	int IsTeleportWeapon(int Index) const;
	int IsTeleportHook(int Index) const;
	int IsTeleCheckpoint(int Index) const;
	bool IsSpeedup(int Index) const;
	int IsTune(int Index) const;
	void GetSpeedup(int Index, wvec2 *pDir, int *pForce, int *pMaxSpeed, int *pType) const;
	int GetSwitchType(int Index) const;
	int GetSwitchNumber(int Index) const;
	int GetSwitchDelay(int Index) const;

	int IsSolid(int x, int y) const;
	bool IsThrough(int x, int y, int OffsetX, int OffsetY, wvec2 Pos0, wvec2 Pos1) const;
	bool IsHookBlocker(int x, int y, wvec2 Pos0, wvec2 Pos1) const;
	int IsWallJump(int Index) const;
	int IsNoLaser(int x, int y) const;
	int IsFrontNoLaser(int x, int y) const;

	int IsTimeCheckpoint(int Index) const;
	int IsFrontTimeCheckpoint(int Index) const;

	int MoverSpeed(int x, int y, wvec2 *pSpeed) const;

	const CLayers *Layers() const { return m_pLayers; }
	const CTile *GameLayer() const { return m_pTiles; }
	const CTeleTile *TeleLayer() const { return m_pTele; }
	const CSpeedupTile *SpeedupLayer() const { return m_pSpeedup; }
	const CTile *FrontLayer() const { return m_pFront; }
	const CSwitchTile *SwitchLayer() const { return m_pSwitch; }
	const CTuneTile *TuneLayer() const { return m_pTune; }

	int m_HighestSwitchNumber;

	/**
	 * Index all teleporter types (in, out and checkpoints)
	 * as one consecutive list.
	 *
	 * @param Number is the teleporter number (one less than what is shown in game)
	 * @param Offset picks the n'th occurrence of that teleporter in the map
	 *
	 * @return The coordinates of the teleporter in the map
	 *         or (-1, -1) if not found
	 */
	wvec2 TeleAllGet(int Number, size_t Offset);

	/**
	 * @param Number is the teleporter number (one less than what is shown in game)
	 * @return The amount of occurrences of that teleporter across all types (in, out, checkpoint)
	 */
	size_t TeleAllSize(int Number);

	const std::vector<wvec2> &TeleIns(int Number) { return m_TeleIns[Number]; }
	const std::vector<wvec2> &TeleOuts(int Number) { return m_TeleOuts[Number]; }
	const std::vector<wvec2> &TeleCheckOuts(int Number) { return m_TeleCheckOuts[Number]; }
	const std::vector<wvec2> &TeleOthers(int Number) { return m_TeleOthers[Number]; }

private:
	CLayers *m_pLayers;

	int m_Width;
	int m_Height;

	CTile *m_pTiles;
	CTeleTile *m_pTele;
	CSpeedupTile *m_pSpeedup;
	CTile *m_pFront;
	CSwitchTile *m_pSwitch;
	CTuneTile *m_pTune;
	CDoorTile *m_pDoor;

	bool m_HoTileTeleEnabled = true;

	// TILE_TELEIN
	std::map<int, std::vector<wvec2>> m_TeleIns;
	// TILE_TELEOUT
	std::map<int, std::vector<wvec2>> m_TeleOuts;
	// TILE_TELECHECKOUT
	std::map<int, std::vector<wvec2>> m_TeleCheckOuts;
	// TILE_TELEINEVIL, TILE_TELECHECK, TILE_TELECHECKIN, TILE_TELECHECKINEVIL
	std::map<int, std::vector<wvec2>> m_TeleOthers;
};

void ThroughOffset(wvec2 Pos0, wvec2 Pos1, int *pOffsetX, int *pOffsetY);
#endif
