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

// Storm: SStrToUpper.cpp
char *SStrToUpper(char *str)
{
	int position;
	char character;

	position = 0;
	while (*str++)
		position++;
	do
	{
		character = str[position];
		if (character > 96 && character < 123)
		{
			character -= 32;
			str[position] = character;
		}
		--position;
	} while (position > -1);
	return str;
}


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
				worldId = unit->GetMapId();
				position.x = unit->GetPositionX();
				position.y = unit->GetPositionY();
				position.z = unit->GetPositionZ();
				facing = unit->GetOrientation();
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

void WorldSession::GmFreezeHandler(WorldPacket &msg)
{
    Player *plyr;
    char name[64];
    int result;

    plyr = 0;
    *name = 0;
    result = 0;
    if (GetSecurity() > 1)
    {
        msg >> name;
        NormalizePlayerName(name);
        if (plyr = sObjectAccessor.FindPlayerByName(name))
        {
            if (plyr->HasAura(9454)) // GM Freeze.
            {
                result = 2;
                plyr->RemoveAura(9454, EFFECT_INDEX_0, 0);
            }
            else
            {
                if (plyr->IsAlive())
                {
                    plyr->CastSpell(plyr, 9454, false, 0);
                    result = 1;
                }
            }
            msg.clear();
            msg.SetOpcode(SMSG_GM_FREEZE);
            msg << result;
			msg.rpos(msg.size());	// Muting ByteBuffer::m_readPos related warning message
            SendPacket(&msg);
        }
        else
            SendPlayerNotFoundFailure();
    }
    else
        SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::GmSilenceHandler(WorldPacket &msg)
{
    Player *plyr;
    char name[64];
    int result;

    plyr = 0;
    *name = 0;
    result = 0;
    if (GetSecurity() > 1)
    {
        msg >> name;
        NormalizePlayerName(name);
        if (plyr = sObjectAccessor.FindPlayerByName(name))
        {
			if (plyr->isGameMaster() || !plyr->IsAlive())
				goto SEND_RESULT;
            if (plyr->GetSession()->m_muteTime > time(0) || plyr->HasAura(1852))
            {
                result = 2;
                plyr->RemoveAura(1852, EFFECT_INDEX_0, 0);
				plyr->GetSession()->m_muteTime = 0;
				LoginDatabase.PExecute("UPDATE account SET mutetime=%llu WHERE id=%d", 0, plyr->GetSession()->GetAccountId());
            }
            else
            {
                if (plyr->IsAlive())
                {
                    plyr->CastSpell(plyr, 1852, false, 0);
                    result = 1;
                }
            }
		SEND_RESULT:
            msg.clear();
            msg.SetOpcode(SMSG_GM_SILENCE);
            msg << result;
			msg.rpos(msg.size());	// Muting ByteBuffer::m_readPos related warning message
            SendPacket(&msg);
        }
        else
            SendPlayerNotFoundFailure();
    }
    else
        SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::GmBroadcastHandler(WorldPacket &msg)
{
	char broadcast[255];

	*broadcast = 0;
	if (GetSecurity() > 1)
	{
		msg >> broadcast;
		if (*broadcast)
			sWorld.SendServerMessage(SERVER_MSG_CUSTOM, broadcast, 0);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::HandleLearnSpell(WorldPacket &msg)
{
	int spell;

	spell = 0;
	if (GetSecurity() > 0)
	{
		msg >> spell;
		if (spell)
		{
			// Make sure we have no duplicate entries in the Spell Book
			if (GetPlayer()->addSpell(spell, true, true, false, false) && GetPlayer()->IsInWorld())
			{
				msg.clear();
				msg.SetOpcode(SMSG_LEARNED_SPELL);
				msg << spell;
				msg.rpos(msg.size());	// Muting ByteBuffer::m_readPos related warning message
				SendPacket(&msg);
			}
		}
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::HandleGmNuke(WorldPacket &msg)
{
	Player *plyr;
	char characterName[64];

	plyr = 0;
	*characterName = 0;
	if (GetSecurity() > 0)
	{
		msg >> characterName;
		NormalizePlayerName(characterName);
		if (plyr = sObjectAccessor.FindPlayerByName(characterName))
			plyr->GetSession()->KickPlayer();
		else
			SendPlayerNotFoundFailure();
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::HandleGmNukeAccount(WorldPacket &msg)
{
	WorldSession *account;
	Player *plyr;
	//char accountName[12];
	unsigned int accountNumber;

	account = 0;
	plyr = 0;
	//*accountName = 0;
	accountNumber = 0;
	if (GetSecurity() > 0)
	{
		//msg >> accountName;
		msg >> accountNumber;
		//SStrToUpper(accountName);
		//account = sWorld.GetSession(accountName);
		account = sWorld.FindSession(accountNumber);
		if (account)
		{
			plyr = account->GetPlayer();
			plyr->GetSession()->KickPlayer();
		}
		else
			SendConsoleMessage("Account not found", WARNING_COLOR);
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}

void WorldSession::HandleSwimCheat(WorldPacket &msg)
{
	int enable;

	if (GetSecurity() > 0)
	{
		msg >> enable;
		GetPlayer()->SetCanFly(enable);
		/*
		MovementInfo movementInfo;
		msg >> enable;
		msg.clear();
		msg.SetOpcode(MSG_MOVE_START_SWIM);
		//msg << (char)enable;
		msg << GetPlayer()->GetPackGUID();             // write guid
		movementInfo.ChangePosition
		(
			GetPlayer()->GetPositionX(),
			GetPlayer()->GetPositionY(),
			(GetPlayer()->GetPositionZ() + 3.0f),
			GetPlayer()->GetOrientation()
		);
		movementInfo.SetMovementFlags((MovementFlags)enable);
		movementInfo.Write(msg);
		msg.rpos(msg.size());	// Muting ByteBuffer::m_readPos related warning message
		SendPacket(&msg);
		*/
	}
	else
		SendNotification(LANG_YOU_NOT_HAVE_PERMISSION);
}
