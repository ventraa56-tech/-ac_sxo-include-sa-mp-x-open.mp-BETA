#if defined _inc_ac_sxo
    #endinput
#endif
#define _inc_ac_sxo

#include <MapAndreas>

/*
    ===================================
        AC-SXO
        Advanced Protection System
    ===================================
*/

static SxoAC_UpdateFlood[MAX_PLAYERS];
static SxoAC_FloodWarnings[MAX_PLAYERS];
static SxoAC_InvalidPackets[MAX_PLAYERS];

static SxoAC_LastShot[MAX_PLAYERS];
static SxoAC_ShotWarnings[MAX_PLAYERS];

static Float:SxoAC_LastX[MAX_PLAYERS];
static Float:SxoAC_LastY[MAX_PLAYERS];
static Float:SxoAC_LastZ[MAX_PLAYERS];

static SxoAC_Tick[MAX_PLAYERS];
static SxoAC_Warnings[MAX_PLAYERS];
static SxoAC_AirTicks[MAX_PLAYERS];
static SxoAC_SpawnProtect[MAX_PLAYERS];

/*
    ===================================
        SETTINGS
    ===================================
*/

#define SXO_AC_AIR_HEIGHT        (5.0)
#define SXO_AC_MAX_DISTANCE      (12.0)
#define SXO_AC_MAX_AIR_TICKS     (4)
#define SXO_AC_MAX_WARNINGS      (3)
#define SXO_AC_UPDATE_INTERVAL   (200)

#define SXO_AC_MIN_SHOT_INTERVAL (85)
#define SXO_AC_MAX_SHOT_WARNINGS (4)

#define SXO_AC_MAX_UPDATES       (180)
#define SXO_AC_MAX_INVALID       (3)

/*
    ===================================
        MESSAGE SYSTEM
    ===================================
*/

stock SxoAC_SendMessage(playerid, const text[])
{
    new str[144];

    format(
        str,
        sizeof(str),
        "{FF0000}[AC-SXO]{FFFFFF} %s",
        text
    );

    SendClientMessage(playerid, -1, str);

    return 1;
}

/*
    ===================================
        RESET SYSTEM
    ===================================
*/

stock SxoAC_Reset(playerid)
{
    SxoAC_Warnings[playerid] = 0;
    SxoAC_AirTicks[playerid] = 0;

    SxoAC_UpdateFlood[playerid] = 0;
    SxoAC_FloodWarnings[playerid] = 0;

    SxoAC_InvalidPackets[playerid] = 0;

    SxoAC_ShotWarnings[playerid] = 0;

    return 1;
}

/*
    ===================================
        MAIN AIRBREAK DETECTION
    ===================================
*/

stock SxoAC_Process(playerid)
{
    if(!IsPlayerConnected(playerid))
        return 1;

    if(GetTickCount() - SxoAC_SpawnProtect[playerid] < 5000)
        return 1;

    if(GetPlayerState(playerid) != PLAYER_STATE_ONFOOT)
        return 1;

    if(GetTickCount() - SxoAC_Tick[playerid] < SXO_AC_UPDATE_INTERVAL)
        return 1;

    if(GetPlayerSurfingVehicleID(playerid) != INVALID_VEHICLE_ID)
        return 1;

    if(GetPlayerSpecialAction(playerid) == SPECIAL_ACTION_USEJETPACK)
        return 1;

    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);

    new Float:dist3D = floatsqroot(
        floatpower(x - SxoAC_LastX[playerid], 2) +
        floatpower(y - SxoAC_LastY[playerid], 2) +
        floatpower(z - SxoAC_LastZ[playerid], 2)
    );

    new Float:groundZ;
    MapAndreas_FindZ_For2DCoord(x, y, groundZ);

    new Float:height = z - groundZ;

    new Float:vx, Float:vy, Float:vz;
    GetPlayerVelocity(playerid, vx, vy, vz);

    new Float:speed = floatsqroot(
        (vx * vx) +
        (vy * vy) +
        (vz * vz)
    );

    new anim = GetPlayerAnimationIndex(playerid);

    if(anim == 1130 ||
       anim == 1132 ||
       anim == 1195 ||
       anim == 1159)
    {
        return 1;
    }

    /*
        AIRBREAK CHECK
    */

    if(height > SXO_AC_AIR_HEIGHT && speed < 0.25)
    {
        SxoAC_AirTicks[playerid]++;
    }
    else
    {
        if(SxoAC_AirTicks[playerid] > 0)
            SxoAC_AirTicks[playerid]--;
    }

    if(dist3D > SXO_AC_MAX_DISTANCE)
    {
        SxoAC_Warnings[playerid]++;
    }

    if(SxoAC_AirTicks[playerid] >= SXO_AC_MAX_AIR_TICKS)
    {
        SxoAC_Warnings[playerid]++;
    }

    /*
        PUNISHMENT
    */

    if(SxoAC_Warnings[playerid] >= SXO_AC_MAX_WARNINGS)
    {
        new str[144];
        new pName[MAX_PLAYER_NAME];

        GetPlayerName(playerid, pName, sizeof(pName));

        format(
            str,
            sizeof(str),
            "[AC-SXO] %s flagged for AirBreak (Dist: %.2f | Height: %.2f)",
            pName,
            dist3D,
            height
        );

        print(str);

        SxoAC_SendMessage(
            playerid,
            "AirBreak detected."
        );

        SetPlayerPos(
            playerid,
            SxoAC_LastX[playerid],
            SxoAC_LastY[playerid],
            SxoAC_LastZ[playerid]
        );

        TogglePlayerControllable(playerid, 0);

        SetTimerEx(
            "SxoAC_Unfreeze",
            1500,
            false,
            "d",
            playerid
        );

        if(SxoAC_Warnings[playerid] >= 5)
        {
            Kick(playerid);
            return 0;
        }
    }

    SxoAC_LastX[playerid] = x;
    SxoAC_LastY[playerid] = y;
    SxoAC_LastZ[playerid] = z;

    SxoAC_Tick[playerid] = GetTickCount();

    return 1;
}

/*
    ===================================
        RAPID FIRE DETECTION
    ===================================
*/

stock SxoAC_ProcessRapidFire(playerid)
{
    new weapon = GetPlayerWeapon(playerid);

    if(weapon < 22 || weapon > 38)
        return 1;

    new tick = GetTickCount();

    new interval = tick - SxoAC_LastShot[playerid];

    if(interval < SXO_AC_MIN_SHOT_INTERVAL)
    {
        SxoAC_ShotWarnings[playerid]++;

        if(SxoAC_ShotWarnings[playerid] >= SXO_AC_MAX_SHOT_WARNINGS)
        {
            new name[MAX_PLAYER_NAME];
            new str[144];

            GetPlayerName(playerid, name, sizeof(name));

            format(
                str,
                sizeof(str),
                "[AC-SXO] %s flagged for RapidFire (%dms)",
                name,
                interval
            );

            print(str);

            SxoAC_SendMessage(
                playerid,
                "RapidFire detected."
            );

            ResetPlayerWeapons(playerid);

            TogglePlayerControllable(playerid, 0);

            SetTimerEx(
                "SxoAC_Unfreeze",
                700,
                false,
                "d",
                playerid
            );

            if(SxoAC_ShotWarnings[playerid] >= 7)
            {
                Kick(playerid);
                return 0;
            }
        }
    }
    else
    {
        if(SxoAC_ShotWarnings[playerid] > 0)
        {
            SxoAC_ShotWarnings[playerid]--;
        }
    }

    SxoAC_LastShot[playerid] = tick;

    return 1;
}

/*
    ===================================
        FLOOD DETECTION
    ===================================
*/

stock SxoAC_ProcessFlood(playerid)
{
    SxoAC_UpdateFlood[playerid]++;

    if(SxoAC_UpdateFlood[playerid] > SXO_AC_MAX_UPDATES)
    {
        SxoAC_FloodWarnings[playerid]++;

        SxoAC_SendMessage(
            playerid,
            "Network flood detected."
        );

        TogglePlayerControllable(playerid, 0);

        SetTimerEx(
            "SxoAC_Unfreeze",
            1200,
            false,
            "d",
            playerid
        );

        if(SxoAC_FloodWarnings[playerid] >= 3)
        {
            new str[144];
            new name[MAX_PLAYER_NAME];

            GetPlayerName(playerid, name, sizeof(name));

            format(
                str,
                sizeof(str),
                "[AC-SXO] %s kicked for packet flood.",
                name
            );

            print(str);

            Kick(playerid);

            return 0;
        }
    }

    return 1;
}

/*
    ===================================
        INVALID PACKET DETECTION
    ===================================
*/

stock SxoAC_ProcessInvalid(playerid)
{
    new weapon = GetPlayerWeapon(playerid);

    if(weapon < 0 || weapon > 46)
    {
        SxoAC_InvalidPackets[playerid]++;

        if(SxoAC_InvalidPackets[playerid] >= SXO_AC_MAX_INVALID)
        {
            SxoAC_SendMessage(
                playerid,
                "Invalid packet detected."
            );

            Kick(playerid);

            return 0;
        }
    }

    return 1;
}

/*
    ===================================
        UNFREEZE
    ===================================
*/

forward SxoAC_Unfreeze(playerid);
public SxoAC_Unfreeze(playerid)
{
    if(IsPlayerConnected(playerid))
    {
        TogglePlayerControllable(playerid, 1);
    }

    return 1;
}

/*
    ===================================
        FLOOD RESET TIMER
    ===================================
*/

forward SxoAC_ResetFlood();
public SxoAC_ResetFlood()
{
    for(new i; i < MAX_PLAYERS; i++)
    {
        SxoAC_UpdateFlood[i] = 0;
    }

    return 1;
}

/*
    ===================================
        CALLBACKS
    ===================================
*/

public OnGameModeInit()
{
    SetTimer(
        "SxoAC_ResetFlood",
        1000,
        true
    );

    #if defined AC_SXO_OnGameModeInit
        return AC_SXO_OnGameModeInit();
    #else
        return 1;
    #endif
}

public OnPlayerSpawn(playerid)
{
    SxoAC_SpawnProtect[playerid] = GetTickCount();

    SxoAC_Reset(playerid);

    #if defined AC_SXO_OnPlayerSpawn
        return AC_SXO_OnPlayerSpawn(playerid);
    #else
        return 1;
    #endif
}

public OnPlayerDisconnect(playerid, reason)
{
    SxoAC_Reset(playerid);

    #if defined AC_SXO_OnPlayerDisconnect
        return AC_SXO_OnPlayerDisconnect(playerid, reason);
    #else
        return 1;
    #endif
}

public OnPlayerUpdate(playerid)
{
    SxoAC_ProcessFlood(playerid);

    SxoAC_ProcessInvalid(playerid);

    SxoAC_Process(playerid);

    #if defined AC_SXO_OnPlayerUpdate
        return AC_SXO_OnPlayerUpdate(playerid);
    #else
        return 1;
    #endif
}

public OnPlayerWeaponShot(
    playerid,
    WEAPON:weaponid,
    BULLET_HIT_TYPE:hittype,
    hitid,
    Float:fX,
    Float:fY,
    Float:fZ
)
{
    SxoAC_ProcessRapidFire(playerid);

    #if defined AC_SXO_OnPlayerWeaponShot
        return AC_SXO_OnPlayerWeaponShot(
            playerid,
            weaponid,
            hittype,
            hitid,
            fX,
            fY,
            fZ
        );
    #else
        return 1;
    #endif
}

/*
    ===================================
        ALS HOOKS
    ===================================
*/

#if defined _ALS_OnGameModeInit
    #undef OnGameModeInit
#else
    #define _ALS_OnGameModeInit
#endif
#define OnGameModeInit AC_SXO_OnGameModeInit

#if defined AC_SXO_OnGameModeInit
    forward AC_SXO_OnGameModeInit();
#endif

#if defined _ALS_OnPlayerUpdate
    #undef OnPlayerUpdate
#else
    #define _ALS_OnPlayerUpdate
#endif
#define OnPlayerUpdate AC_SXO_OnPlayerUpdate

#if defined AC_SXO_OnPlayerUpdate
    forward AC_SXO_OnPlayerUpdate(playerid);
#endif

#if defined _ALS_OnPlayerSpawn
    #undef OnPlayerSpawn
#else
    #define _ALS_OnPlayerSpawn
#endif
#define OnPlayerSpawn AC_SXO_OnPlayerSpawn

#if defined AC_SXO_OnPlayerSpawn
    forward AC_SXO_OnPlayerSpawn(playerid);
#endif

#if defined _ALS_OnPlayerDisconnect
    #undef OnPlayerDisconnect
#else
    #define _ALS_OnPlayerDisconnect
#endif
#define OnPlayerDisconnect AC_SXO_OnPlayerDisconnect

#if defined AC_SXO_OnPlayerDisconnect
    forward AC_SXO_OnPlayerDisconnect(playerid, reason);
#endif

#if defined _ALS_OnPlayerWeaponShot
    #undef OnPlayerWeaponShot
#else
    #define _ALS_OnPlayerWeaponShot
#endif
#define OnPlayerWeaponShot AC_SXO_OnPlayerWeaponShot

#if defined AC_SXO_OnPlayerWeaponShot
    forward AC_SXO_OnPlayerWeaponShot(
        playerid,
        WEAPON:weaponid,
        BULLET_HIT_TYPE:hittype,
        hitid,
        Float:fX,
        Float:fY,
        Float:fZ
    );
#endif