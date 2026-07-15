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

	// Far-lands rendering: absolute float world coords make MapScreen collapse
	// (center±half == center). Use the same render origin as tees and fill with
	// edge-clamped tiles so infinite border stays visible.
	m_Params.m_RenderOrigin = GameClient()->RenderOrigin();

	const vec2 C = m_Params.m_Center;
	float MapMaxX = 0.0f;
	float MapMaxY = 0.0f;
	if(m_pLayers && m_pLayers->GameLayer())
	{
		MapMaxX = (float)m_pLayers->GameLayer()->m_Width * 32.0f;
		MapMaxY = (float)m_pLayers->GameLayer()->m_Height * 32.0f;
	}
	const float MarginPx = (float)BorderRenderDistance * 32.0f;
	const bool OutsideMap =
		!std::isfinite(C.x) || !std::isfinite(C.y) ||
		C.x < -MarginPx || C.y < -MarginPx ||
		C.x > MapMaxX + MarginPx || C.y > MapMaxY + MarginPx;
	// ~1e5 px: float already loses view width around the camera
	const bool FloatUnsafe = std::abs(C.x) > 100000.0f || std::abs(C.y) > 100000.0f;
	const bool HasOrigin = m_Params.m_RenderOrigin.x != 0.0f || m_Params.m_RenderOrigin.y != 0.0f;
	m_Params.m_FarLands = OutsideMap || FloatUnsafe || HasOrigin;

	// Prefer full i128 character pos for edge sampling (float camera loses tile index far out)
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
		m_Params.m_CamTileX = 1.0e300; // force far-right/bottom edge sample
		m_Params.m_CamTileY = 1.0e300;
	}

	m_MapRenderer.Render(m_Params);
}
