/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "entity.h"

#include <game/collision.h>

//////////////////////////////////////////////////
// Entity
//////////////////////////////////////////////////
CEntity::CEntity(CGameWorld *pGameWorld, int ObjType, wvec2 Pos, int ProximityRadius)
{
	m_pGameWorld = pGameWorld;

	m_ObjType = ObjType;
	m_Pos = Pos;
	m_ProximityRadius = ProximityRadius;

	m_MarkedForDestroy = false;
	m_Id = -1;

	m_pPrevTypeEntity = nullptr;
	m_pNextTypeEntity = nullptr;
	m_SnapTicks = -1;

	// DDRace
	m_pParent = nullptr;
	m_pChild = nullptr;
	m_DestroyTick = -1;
	m_LastRenderTick = -1;
}

CEntity::~CEntity()
{
	if(GameWorld())
		GameWorld()->RemoveEntity(this);
}

bool CEntity::GameLayerClipped(wvec2 CheckPos)
{
	const int64_t Tx = CheckPos.x.to_int64() / 32;
	const int64_t Ty = CheckPos.y.to_int64() / 32;
	return Tx < -200 || Tx > Collision()->GetWidth() + 200 ||
	       Ty < -200 || Ty > Collision()->GetHeight() + 200;
}
