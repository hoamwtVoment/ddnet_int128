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
	const i128 Tx = CheckPos.x.to_i128_pixels() / I128(32);
	const i128 Ty = CheckPos.y.to_i128_pixels() / I128(32);
	const i128 W = I128(Collision()->GetWidth());
	const i128 H = I128(Collision()->GetHeight());
	return Tx < I128(-200) || Tx > W + I128(200) ||
	       Ty < I128(-200) || Ty > H + I128(200);
}
