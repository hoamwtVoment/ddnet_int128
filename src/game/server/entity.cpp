/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "entity.h"

#include "gamecontext.h"
#include "player.h"

#include <game/server/gameworld.h>

//////////////////////////////////////////////////
// Entity
//////////////////////////////////////////////////
CEntity::CEntity(CGameWorld *pGameWorld, int ObjType, bool SnapFreeId, wvec2 Pos, int ProximityRadius)
{
	m_pGameWorld = pGameWorld;
	m_pCCollision = GameServer()->Collision();

	m_ObjType = ObjType;
	m_Pos = Pos;
	m_ProximityRadius = ProximityRadius;

	m_MarkedForDestroy = false;
	if(SnapFreeId)
		m_Id = Server()->SnapNewId();

	m_pPrevTypeEntity = nullptr;
	m_pNextTypeEntity = nullptr;
}

CEntity::~CEntity()
{
	GameWorld()->RemoveEntity(this);
	if(m_Id.has_value())
		Server()->SnapFreeId(m_Id.value());
}

bool CEntity::NetworkClipped(int SnappingClient) const
{
	return ::NetworkClipped(m_pGameWorld->GameServer(), SnappingClient, m_Pos);
}

bool CEntity::NetworkClipped(int SnappingClient, wvec2 CheckPos) const
{
	return ::NetworkClipped(m_pGameWorld->GameServer(), SnappingClient, CheckPos);
}

bool CEntity::NetworkClippedLine(int SnappingClient, wvec2 StartPos, wvec2 EndPos) const
{
	return ::NetworkClippedLine(m_pGameWorld->GameServer(), SnappingClient, StartPos, EndPos);
}

bool CEntity::GameLayerClipped(wvec2 CheckPos)
{
	return round_to_int(CheckPos.x) / 32 < -200 || round_to_int(CheckPos.x) / 32 > GameServer()->Collision()->GetWidth() + 200 ||
	       round_to_int(CheckPos.y) / 32 < -200 || round_to_int(CheckPos.y) / 32 > GameServer()->Collision()->GetHeight() + 200;
}

bool CEntity::GetNearestAirPos(wvec2 Pos, wvec2 PrevPos, wvec2 *pOutPos)
{
	for(int k = 0; k < 16 && GameServer()->Collision()->CheckPoint(Pos); k++)
	{
		Pos -= normalize(PrevPos - Pos);
	}

	wvec2 PosInBlock = wvec2(round_to_int(Pos.x) % 32, round_to_int(Pos.y) % 32);
	wvec2 BlockCenter = wvec2(round_to_int(Pos.x), round_to_int(Pos.y)) - PosInBlock + wvec2(16.0f, 16.0f);

	*pOutPos = wvec2(BlockCenter.x + (PosInBlock.x < 16 ? -2.0f : 1.0f), Pos.y);
	if(!GameServer()->Collision()->TestBox(*pOutPos, CCharacterCore::PhysicalSizeVec2()))
		return true;

	*pOutPos = wvec2(Pos.x, BlockCenter.y + (PosInBlock.y < 16 ? -2.0f : 1.0f));
	if(!GameServer()->Collision()->TestBox(*pOutPos, CCharacterCore::PhysicalSizeVec2()))
		return true;

	*pOutPos = wvec2(BlockCenter.x + (PosInBlock.x < 16 ? -2.0f : 1.0f),
		BlockCenter.y + (PosInBlock.y < 16 ? -2.0f : 1.0f));
	return !GameServer()->Collision()->TestBox(*pOutPos, CCharacterCore::PhysicalSizeVec2());
}

bool CEntity::GetNearestAirPosPlayer(wvec2 PlayerPos, wvec2 *pOutPos)
{
	for(int Distance = 5; Distance >= -1; Distance--)
	{
		*pOutPos = wvec2(PlayerPos.x, PlayerPos.y - Distance);
		if(!GameServer()->Collision()->TestBox(*pOutPos, CCharacterCore::PhysicalSizeVec2()))
		{
			return true;
		}
	}
	return false;
}

bool NetworkClipped(const CGameContext *pGameServer, int SnappingClient, wvec2 CheckPos)
{
	if(SnappingClient == SERVER_DEMO_CLIENT || pGameServer->m_apPlayers[SnappingClient]->m_ShowAll)
		return false;

	float dx = pGameServer->m_apPlayers[SnappingClient]->m_ViewPos.x - CheckPos.x;
	if(absolute(dx) > pGameServer->m_apPlayers[SnappingClient]->m_ShowDistance.x)
		return true;

	float dy = pGameServer->m_apPlayers[SnappingClient]->m_ViewPos.y - CheckPos.y;
	return absolute(dy) > pGameServer->m_apPlayers[SnappingClient]->m_ShowDistance.y;
}

bool NetworkClippedLine(const CGameContext *pGameServer, int SnappingClient, wvec2 StartPos, wvec2 EndPos)
{
	if(SnappingClient == SERVER_DEMO_CLIENT || pGameServer->m_apPlayers[SnappingClient]->m_ShowAll)
		return false;

	wvec2 &ViewPos = pGameServer->m_apPlayers[SnappingClient]->m_ViewPos;
	wvec2 &ShowDistance = pGameServer->m_apPlayers[SnappingClient]->m_ShowDistance;

	wvec2 DistanceToLine, ClosestPoint;
	if(closest_point_on_line(StartPos, EndPos, ViewPos, ClosestPoint))
	{
		DistanceToLine = ViewPos - ClosestPoint;
	}
	else
	{
		// No line section was passed but two equal points
		DistanceToLine = ViewPos - StartPos;
	}
	float ClippDistance = std::max(ShowDistance.x, ShowDistance.y);
	return (absolute(DistanceToLine.x) > ClippDistance || absolute(DistanceToLine.y) > ClippDistance);
}
