/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "maplayers.h"

#include <base/math.h>

#include <game/client/gameclient.h>
#include <game/gamecore.h>
#include <game/localization.h>
#include <game/map/render_layer.h>

#include <cmath>

CMapLayers::CMapLayers(ERenderType Type, bool OnlineOnly)
{
	m_Type = Type;
	m_OnlineOnly = OnlineOnly;

	// static parameters for ingame rendering
	m_Params.m_RenderType = m_Type;
	m_Params.m_RenderInvalidTiles = false;
	m_Params.m_TileAndQuadBuffering = true;
	m_Params.m_RenderTileBorder = true;
}

void CMapLayers::OnInit()
{
	m_pLayers = Layers();
	m_pImages = &GameClient()->m_MapImages;
	m_MapRenderer.Clear();
}

CCamera *CMapLayers::GetCurCamera()
{
	return &GameClient()->m_Camera;
}

void CMapLayers::OnMapLoad()
{
	FRenderUploadCallback FRenderCallback = [&](const char *pTitle, const char *pMessage, int IncreaseCounter) { GameClient()->m_Menus.RenderLoading(pTitle, pMessage, IncreaseCounter); };
	auto FRenderCallbackOptional = std::make_optional<FRenderUploadCallback>(FRenderCallback);

	const char *pLoadingTitle = Localize("Loading map");
	const char *pLoadingMessage = Localize("Uploading map data to GPU");
	GameClient()->m_Menus.RenderLoading(pLoadingTitle, pLoadingMessage, 0);

	// can't do that in CMapLayers::OnInit, because some of this interfaces are not available yet
	m_MapRenderer.OnInit(Graphics(), TextRender(), RenderMap());

	m_EnvEvaluator = CEnvelopeState(m_pLayers->Map(), m_OnlineOnly);
	m_EnvEvaluator.OnInterfacesInit(GameClient());
	m_MapRenderer.Load(m_Type, m_pLayers, m_pImages, &m_EnvEvaluator, FRenderCallbackOptional);
}

void CMapLayers::OnRender()
{
	if(m_OnlineOnly && Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	// dynamic parameters for ingame rendering
	m_Params.m_EntityOverlayVal = m_Type == RENDERTYPE_FULL_DESIGN ? 0 : g_Config.m_ClOverlayEntities;
	m_Params.m_Center = GetCurCamera()->m_Center;
	m_Params.m_Zoom = GetCurCamera()->m_Zoom;
	m_Params.m_RenderText = g_Config.m_ClTextEntities;
	m_Params.m_DebugRenderGroupClips = g_Config.m_DbgRenderGroupClips;
	m_Params.m_DebugRenderQuadClips = g_Config.m_DbgRenderQuadClips;
	m_Params.m_DebugRenderClusterClips = g_Config.m_DbgRenderClusterClips;
	m_Params.m_DebugRenderTileClips = g_Config.m_DbgRenderTileClips;

	// Far-lands: only when camera is actually outside the map tile rect (or float
	// view would collapse). Do NOT key off RenderOrigin alone — that wrongly forced
	// the whole map into edge-fill + kill wash.
	m_Params.m_RenderOrigin = GameClient()->RenderOrigin();

	const vec2 C = m_Params.m_Center;
	int MapW = 0;
	int MapH = 0;
	if(m_pLayers && m_pLayers->GameLayer())
	{
		MapW = m_pLayers->GameLayer()->m_Width;
		MapH = m_pLayers->GameLayer()->m_Height;
	}

	// Prefer i128 character pos for tile sampling when available
	if(GameClient()->m_Snap.m_pLocalCharacter)
	{
		m_Params.m_CamTileX = i128_to_double(CharacterNetPosX(GameClient()->m_Snap.m_pLocalCharacter)) / 32.0;
		m_Params.m_CamTileY = i128_to_double(CharacterNetPosY(GameClient()->m_Snap.m_pLocalCharacter)) / 32.0;
	}
	else if(std::isfinite(C.x) && std::isfinite(C.y))
	{
		m_Params.m_CamTileX = (double)C.x / 32.0;
		m_Params.m_CamTileY = (double)C.y / 32.0;
	}
	else
	{
		m_Params.m_CamTileX = 0.0;
		m_Params.m_CamTileY = 0.0;
	}

	const bool CamFinite = std::isfinite(m_Params.m_CamTileX) && std::isfinite(m_Params.m_CamTileY);
	const bool OutsideMapTiles = MapW > 0 && MapH > 0 && CamFinite &&
				     (m_Params.m_CamTileX < 0.0 || m_Params.m_CamTileY < 0.0 ||
					     m_Params.m_CamTileX >= (double)MapW || m_Params.m_CamTileY >= (double)MapH);
	// ~3e6 px (~1e5 tiles): absolute MapScreen loses view width
	const bool FloatUnsafe = (std::isfinite(C.x) && std::abs(C.x) > 3000000.0f) ||
				 (std::isfinite(C.y) && std::abs(C.y) > 3000000.0f) ||
				 !std::isfinite(C.x) || !std::isfinite(C.y);
	m_Params.m_FarLands = OutsideMapTiles || FloatUnsafe;

	// When far, keep render origin for relative MapScreen even if UpdateRenderOrigin lagged
	if(m_Params.m_FarLands && m_Params.m_RenderOrigin.x == 0.0f && m_Params.m_RenderOrigin.y == 0.0f &&
		std::isfinite(C.x) && std::isfinite(C.y))
	{
		m_Params.m_RenderOrigin = vec2(std::floor(C.x + 0.5f), std::floor(C.y + 0.5f));
	}

	m_MapRenderer.Render(m_Params);
}
