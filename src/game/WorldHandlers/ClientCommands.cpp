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

void WorldSession::WorldTeleportHandler(WorldPacket &msg)
{
	int time;
	int mapId;
	C3Vector position;
	float facing;

	msg >> time;
	msg >> mapId;
	msg >> position.x;
	msg >> position.y;
	msg >> position.z;
	msg >> facing;

	if (GetSecurity() > SEC_PLAYER)
		GetPlayer()->TeleportTo(mapId, position.x, position.y, position.z, facing, TELE_TO_GM_MODE);
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
	if (GetSecurity() > SEC_PLAYER)
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

void WorldSession::ReverseWhoIsHandler(WorldPacket &msg)
{
	char accountName[64];
	char characterName[48];
	int result;
	int online;
	unsigned int numcharacters;

	*accountName = 0;
	*characterName = 0;
	result = 0;
	online = -1;
	numcharacters = 0;
	if (GetSecurity())
	{
		msg >> accountName;
		msg.clear();
		msg.SetOpcode(SMSG_RWHOIS);
		for (size_t i = strlen(accountName); i > 0; i--)
		{
			*(accountName + i) = toupper(*(accountName + i));
		}
		QueryResult *accountquery = LoginDatabase.PQuery("SELECT id FROM account WHERE username = '%s'", accountName);
		if (accountquery)
		{
			result = (*accountquery)[0].GetInt32();
			QueryResult *characterquery = CharacterDatabase.PQuery("SELECT name FROM characters WHERE account = '%d'", result);
			if (characterquery)
			{
				result = 1;
				numcharacters = characterquery->GetRowCount();
				msg << result;
				msg << accountName;
				msg << numcharacters;
				msg << result;
				result = msg.wpos() - 4;
				for (unsigned int i = 0; i < numcharacters; i++)
				{
					Field *field = characterquery->Fetch();
					strcpy(characterName, field[0].GetString());
					strcpy(accountName, characterName);
					msg << accountName;
					characterquery->NextRow();
					if (sObjectAccessor.FindPlayerByName(characterName))
						online = i;
				}
				msg.put(result, online);
			}
			else
			{
				result = -1;
				msg << result;
			}
			delete characterquery;
		}
		else
			msg << result;
		msg.rpos(msg.size());	// Muting ByteBuffer::m_readPos related warning message
		SendPacket(&msg);
		delete accountquery;
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::WhoIsHandler(WorldPacket &msg)
{
	char characterName[48];

	*characterName = 0;
	if (GetSecurity() > *characterName)
	{
		msg >> characterName;
		NormalizePlayerName(characterName);
		QueryResult *accountquery = CharacterDatabase.PQuery("SELECT account FROM characters WHERE name = '%s'", characterName);
		if (accountquery)
		{
			QueryResult *result = LoginDatabase.PQuery("SELECT username FROM account WHERE id = '%u'", (*accountquery)[0].GetInt32());
			if (result)
				strcpy(characterName, (*result)[0].GetString());
			msg.clear();
			msg.SetOpcode(SMSG_WHOIS);
			msg << characterName;
			msg.rpos(msg.size());	// Muting ByteBuffer::m_readPos related warning message
			SendPacket(&msg);
			delete result;
		}
		else
			SendPlayerNotFoundFailure();
		delete accountquery;
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::HandleTeleportToUnit(WorldPacket &msg)
{
	Player *plyr;
	Unit *unit;
	C3Vector position;
	unsigned int worldId;
	float facing;
	char characterName[64];
	int summoning;

	summoning = 0;
	characterName[0] = 0;
	if (GetSecurity() > SEC_PLAYER)
	{
		msg >> characterName;
		if (!*characterName)
			SendPlayerNotFoundFailure();
		else
		{
			NormalizePlayerName(characterName);
			plyr = GetPlayer();
			unit = sObjectMgr.GetPlayer(characterName);
			if (!unit)
				SendPlayerNotFoundFailure();
			else
			{
				worldId = plyr->GetMapId();
				position.x = plyr->GetPositionX();
				position.y = plyr->GetPositionY();
				position.z = plyr->GetPositionZ();
				facing = plyr->GetOrientation();
				summoning = plyr->TeleportTo(worldId, position.x, position.y, position.z, facing, TELE_TO_GM_MODE, true);
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

void WorldSession::GmResurrectHandler(WorldPacket &msg)
{
	Player *plyr;
	char characterName[64];
	int result;

	*characterName = 0;
	result = 0;
	if (GetSecurity())
	{
		msg >> characterName;
		if (!*characterName)
			SendPlayerNotFoundFailure();
		else
		{
			NormalizePlayerName(characterName);
			plyr = sObjectMgr.GetPlayer(characterName);
			if (!plyr)
				SendPlayerNotFoundFailure();
			else
			{
				if (plyr->IsDead())
				{
					plyr->ResurrectPlayer(100.0f, false);
					result = 1;
				}
				msg.clear();
				msg.SetOpcode(SMSG_GM_RESURRECT);
				msg << result;
				msg.rpos(msg.size());	// Muting ByteBuffer::m_readPos related warning message
				SendPacket(&msg);
			}
		}
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}
