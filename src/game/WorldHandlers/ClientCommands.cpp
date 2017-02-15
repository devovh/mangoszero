#include <zlib.h>
#include "Common.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Database/DatabaseImpl.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "Player.h"
#include "World.h"
#include "GuildMgr.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include "Auth/BigNumber.h"
#include "Auth/Sha1.h"
#include "UpdateData.h"
#include "LootMgr.h"
#include "Chat.h"
#include "ScriptMgr.h"
#include "ObjectAccessor.h"
#include "Object.h"
#include "BattleGround/BattleGround.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "Pet.h"
#include "SocialMgr.h"

void WorldSession::HandleWorldTeleportOpcode(WorldPacket& recv_data)
{
	DEBUG_LOG("WORLD: Received opcode CMSG_WORLD_TELEPORT from %s", GetPlayer()->GetGuidStr().c_str());

	// write in client console: worldport 469 452 6454 2536 180 or /console worldport 469 452 6454 2536 180
	// Received opcode CMSG_WORLD_TELEPORT
	// Time is ***, map=469, x=452.000000, y=6454.000000, z=2536.000000, orient=3.141593

	uint32 time;
	uint32 mapid;
	float PositionX;
	float PositionY;
	float PositionZ;
	float Orientation;

	recv_data >> time;                                      // time in m.sec.
	recv_data >> mapid;
	recv_data >> PositionX;
	recv_data >> PositionY;
	recv_data >> PositionZ;
	recv_data >> Orientation;                               // o (3.141593 = 180 degrees)

	if (GetSecurity() >= SEC_ADMINISTRATOR)
		GetPlayer()->TeleportTo(mapid, PositionX, PositionY, PositionZ, Orientation);
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::GmSummonHandler(WorldPacket &msg)
{
	Player *plyr1;
	Player *plyr2;
	C3Vector position;
	unsigned int worldId;
	float facing;
	char characterName[64];
	int summoning;

	summoning = 0;
	characterName[0] = 0;
	if (GetSecurity() > 0)
	{
		msg >> characterName;
		if (!*characterName)
			SendPlayerNotFoundFailure();
		else
		{
			NormalizePlayerName(characterName);
			plyr1 = GetPlayer();
			plyr2 = sObjectMgr.GetPlayer(characterName);
			if (!plyr2)
				SendPlayerNotFoundFailure();
			else
			{
				worldId = plyr1->GetMapId();
				position.x = plyr1->GetPositionX();
				position.y = plyr1->GetPositionY();
				position.z = plyr1->GetPositionZ();
				facing = plyr1->GetOrientation();
				summoning = plyr2->TeleportTo(worldId, position.x, position.y, position.z, facing, TELE_TO_GM_MODE, true);
				msg.clear();
				msg.SetOpcode(MSG_GM_SUMMON);
				msg << summoning;
				msg.rpos(msg.size());	// Muting ByteBuffer::m_readPos related warning message
				SendPacket(&msg);
			}
		}
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}
