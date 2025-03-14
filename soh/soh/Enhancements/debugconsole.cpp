#include "debugconsole.h"
#include <ImGuiImpl.h>
#include <Utils.h>
#include "savestates.h"
#include <Console.h>

#include <vector>
#include <string>
#include "soh/OTRGlobals.h"
#include <soh/Enhancements/item-tables/ItemTableManager.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/cosmetics/CosmeticsEditor.h"
#include "soh/Enhancements/audio/AudioEditor.h"

#define Path _Path
#define PATH_HACK
#include <Utils/StringHelper.h>

#include <Window.h>
#include <ImGui/imgui_internal.h>
#undef PATH_HACK
#undef Path

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
extern PlayState* gPlayState;
}

#include <libultraship/bridge.h>

#define CMD_REGISTER SohImGui::GetConsole()->AddCommand

static bool ActorSpawnHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if ((args.size() != 9) && (args.size() != 3) && (args.size() != 6)) {
        SohImGui::GetConsole()->SendErrorMessage("Not enough arguments passed to actorspawn");
        return CMD_FAILED;
    }

    if (gPlayState == nullptr) {
        SohImGui::GetConsole()->SendErrorMessage("PlayState == nullptr");
        return CMD_FAILED;
    }

    Player* player = GET_PLAYER(gPlayState);
    PosRot spawnPoint;
    const s16 actorId = std::stoi(args[1]);
    const s16 params = std::stoi(args[2]);

    spawnPoint = player->actor.world;

    switch (args.size()) {
        case 9:
            if (args[6][0] != ',') {
                spawnPoint.rot.x = std::stoi(args[6]);
            }
            if (args[7][0] != ',') {
                spawnPoint.rot.y = std::stoi(args[7]);
            }
            if (args[8][0] != ',') {
                spawnPoint.rot.z = std::stoi(args[8]);
            }
        case 6:
            if (args[3][0] != ',') {
                spawnPoint.pos.x = std::stoi(args[3]);
            }
            if (args[4][0] != ',') {
                spawnPoint.pos.y = std::stoi(args[4]);
            }
            if (args[5][0] != ',') {
                spawnPoint.pos.z = std::stoi(args[5]);
            }
    }

    if (Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorId, spawnPoint.pos.x, spawnPoint.pos.y, spawnPoint.pos.z,
                    spawnPoint.rot.x, spawnPoint.rot.y, spawnPoint.rot.z, params, 0) == NULL) {
        SohImGui::GetConsole()->SendErrorMessage("Failed to spawn actor. Actor_Spawn returned NULL");
        return CMD_FAILED;
    }
    return CMD_SUCCESS;
}

static bool KillPlayerHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>&) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::SetPlayerHealth();
    effect->parameters[0] = 0;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] You've met with a terrible fate, haven't you?");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not kill player.");
        return CMD_FAILED;
    }
}

static bool SetPlayerHealthHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    int health;

    try {
        health = std::stoi(args[1]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Health value must be an integer.");
        return CMD_FAILED;
    }

    if (health < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Health value must be a positive integer");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::SetPlayerHealth();
    effect->parameters[0] = health;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Player health updated to %d", health);
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not set player health.");
        return CMD_FAILED;
    }
}

static bool LoadSceneHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>&) {
    gSaveContext.respawnFlag = 0;
    gSaveContext.seqId = 0xFF;
    gSaveContext.gameMode = 0;

    return CMD_SUCCESS;
}

static bool RupeeHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return CMD_FAILED;
    }

    int rupeeAmount;
    try {
        rupeeAmount = std::stoi(args[1]);
    }
    catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Rupee count must be an integer.");
        return CMD_FAILED;
    }

    if (rupeeAmount < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Rupee count must be positive");
        return CMD_FAILED;
    }

   gSaveContext.rupees = rupeeAmount;

    SohImGui::GetConsole()->SendInfoMessage("Set rupee count to %u", rupeeAmount);
    return CMD_SUCCESS;
}

static bool SetPosHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string> args) {
    if (gPlayState == nullptr) {
        SohImGui::GetConsole()->SendErrorMessage("PlayState == nullptr");
        return CMD_FAILED;
    }

    Player* player = GET_PLAYER(gPlayState);

    if (args.size() == 1) {
        SohImGui::GetConsole()->SendInfoMessage("Player position is [ %.2f, %.2f, %.2f ]", player->actor.world.pos.x,
                                            player->actor.world.pos.y,
             player->actor.world.pos.z);
        return CMD_SUCCESS;
    }
    if (args.size() < 4)
        return CMD_FAILED;

    player->actor.world.pos.x = std::stof(args[1]);
    player->actor.world.pos.y = std::stof(args[2]);
    player->actor.world.pos.z = std::stof(args[3]);

    SohImGui::GetConsole()->SendInfoMessage("Set player position to [ %.2f, %.2f, %.2f ]", player->actor.world.pos.x,
                                        player->actor.world.pos.y,
         player->actor.world.pos.z);
    return CMD_SUCCESS;
}

static bool ResetHandler(std::shared_ptr<Ship::Console> Console, std::vector<std::string> args) {
    if (gPlayState == nullptr) {
        SohImGui::GetConsole()->SendErrorMessage("PlayState == nullptr");
        return CMD_FAILED;
    }

    SET_NEXT_GAMESTATE(&gPlayState->state, TitleSetup_Init, GameState);
    gPlayState->state.running = false;
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnExitGame>(gSaveContext.fileNum);
    return CMD_SUCCESS;
}

const static std::map<std::string, uint16_t> ammoItems{ 
    { "sticks", ITEM_STICK }, { "nuts", ITEM_NUT },
    { "bombs", ITEM_BOMB },   { "seeds", ITEM_SLINGSHOT },
    { "arrows", ITEM_BOW },   { "bombchus", ITEM_BOMBCHU },
    { "beans", ITEM_BEAN }
};

static bool AddAmmoHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    int amount;

    try {
        amount = std::stoi(args[2]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("Ammo count must be an integer");
        return CMD_FAILED;
    }

    if (amount < 0) {
        SohImGui::GetConsole()->SendErrorMessage("Ammo count must be positive");
        return CMD_FAILED;
    }

    const auto& it = ammoItems.find(args[1]);
    if (it == ammoItems.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Invalid ammo type. Options are 'sticks', 'nuts', 'bombs', 'seeds', 'arrows', 'bombchus' and 'beans'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::AddOrTakeAmmo();
    effect->parameters[0] = amount;
    effect->parameters[1] = it->second;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Added ammo.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not add ammo.");
        return CMD_FAILED;
    }
}

static bool TakeAmmoHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    int amount;

    try {
        amount = std::stoi(args[2]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("Ammo count must be an integer");
        return CMD_FAILED;
    }

    if (amount < 0) {
        SohImGui::GetConsole()->SendErrorMessage("Ammo count must be positive");
        return CMD_FAILED;
    }

    const auto& it = ammoItems.find(args[1]);
    if (it == ammoItems.end()) {
        SohImGui::GetConsole()->SendErrorMessage(
            "Invalid ammo type. Options are 'sticks', 'nuts', 'bombs', 'seeds', 'arrows', 'bombchus' and 'beans'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::AddOrTakeAmmo();
    effect->parameters[0] = -amount;
    effect->parameters[1] = it->second;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Took ammo.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not take ammo.");
        return CMD_FAILED;
    }
}

const static std::map<std::string, uint16_t> bottleItems{
    { "green_potion", ITEM_POTION_GREEN }, { "red_potion", ITEM_POTION_RED }, { "blue_potion", ITEM_POTION_BLUE },
    { "milk", ITEM_MILK },                 { "half_milk", ITEM_MILK_HALF },   { "fairy", ITEM_FAIRY },
    { "bugs", ITEM_BUG },                  { "fish", ITEM_FISH },             { "poe", ITEM_POE },
    { "big_poe", ITEM_BIG_POE },           { "blue_fire", ITEM_BLUE_FIRE },   { "rutos_letter", ITEM_LETTER_RUTO },
};

static bool BottleHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    unsigned int slot;
    try {
        slot = std::stoi(args[2]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Bottle slot must be an integer.");
        return CMD_FAILED;
    }

    if ((slot < 1) || (slot > 4)) {
        SohImGui::GetConsole()->SendErrorMessage("Invalid slot passed");
        return CMD_FAILED;
    }

    const auto& it = bottleItems.find(args[1]);

    if (it ==  bottleItems.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Invalid item passed");
        return CMD_FAILED;
    }

    // I dont think you can do OOB with just this
    gSaveContext.inventory.items[0x11 + slot] = it->second;

    return CMD_SUCCESS;
}

static bool BHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    gSaveContext.equips.buttonItems[0] = std::stoi(args[1]);
    return CMD_SUCCESS;
}

static bool ItemHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    gSaveContext.inventory.items[std::stoi(args[1])] = std::stoi(args[2]);

    return CMD_SUCCESS;
}

static bool GiveItemHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string> args) {
    if (args.size() < 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    GetItemEntry getItemEntry = GET_ITEM_NONE;

    if (args[1].compare("vanilla") == 0) {
        getItemEntry = ItemTableManager::Instance->RetrieveItemEntry(MOD_NONE, std::stoi(args[2]));
    } else if (args[1].compare("randomizer") == 0) {
        getItemEntry = ItemTableManager::Instance->RetrieveItemEntry(MOD_RANDOMIZER, std::stoi(args[2]));
    } else {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Invalid argument passed, must be 'vanilla' or 'randomizer'");
        return CMD_FAILED;
    }

    GiveItemEntryWithoutActor(gPlayState, getItemEntry);

    return CMD_SUCCESS;
}

static bool EntranceHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    unsigned int entrance;

    try {
        entrance = std::stoi(args[1], nullptr, 16);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Entrance value must be a Hex number.");
        return CMD_FAILED;
    }

    gPlayState->nextEntranceIndex = entrance;
    gPlayState->sceneLoadFlag = 0x14;
    gPlayState->fadeTransition = 11;
    gSaveContext.nextTransitionType = 11;
}

static bool VoidHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (gPlayState != nullptr) {
            gSaveContext.respawn[RESPAWN_MODE_DOWN].tempSwchFlags = gPlayState->actorCtx.flags.tempSwch;
            gSaveContext.respawn[RESPAWN_MODE_DOWN].tempCollectFlags = gPlayState->actorCtx.flags.tempCollect;
            gSaveContext.respawnFlag = 1;
            gPlayState->sceneLoadFlag = 0x14;
            gPlayState->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex;
            gPlayState->fadeTransition = 2;
            gSaveContext.nextTransitionType = 2;
    } else {
        SohImGui::GetConsole()->SendErrorMessage("gPlayState == nullptr");
        return CMD_FAILED;
    }
    return CMD_SUCCESS;
}

static bool ReloadHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (gPlayState != nullptr) {
        gPlayState->nextEntranceIndex = gSaveContext.entranceIndex;
        gPlayState->sceneLoadFlag = 0x14;
        gPlayState->fadeTransition = 11;
        gSaveContext.nextTransitionType = 11;
    } else {
        SohImGui::GetConsole()->SendErrorMessage("gPlayState == nullptr");
        return CMD_FAILED;
    }
    return CMD_SUCCESS;
}

const static std::map<std::string, uint16_t> fw_options {
    { "clear", 0}, {"warp", 1}, {"backup", 2}
};

static bool FWHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    const auto& it = fw_options.find(args[1]);
    if (it == fw_options.end()) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Invalid option. Options are 'clear', 'warp', 'backup'");
        return CMD_FAILED;
    }
    
    if (gPlayState != nullptr) {
        FaroresWindData clear = {};
        switch(it->second) {
            case 0: //clear
                gSaveContext.fw = clear;
                SohImGui::GetConsole()->SendInfoMessage("[SOH] Farore's wind point cleared! Reload scene to take effect.");
                return CMD_SUCCESS;
                break;
            case 1: //warp
                if (gSaveContext.respawn[RESPAWN_MODE_TOP].data > 0) {
                    gPlayState->sceneLoadFlag = 0x14;
                    gPlayState->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_TOP].entranceIndex;
                    gPlayState->fadeTransition = 5;
                } else {
                    SohImGui::GetConsole()->SendErrorMessage("Farore's wind not set!");
                    return CMD_FAILED;
                }
                return CMD_SUCCESS;
                break;
            case 2: //backup
                if (CVarGetInteger("gBetterFW", 0)) {
                    gSaveContext.fw = gSaveContext.backupFW;
                    gSaveContext.fw.set = 1;
                    SohImGui::GetConsole()->SendInfoMessage("[SOH] Backup FW data copied! Reload scene to take effect.");
                    return CMD_SUCCESS;
                } else {
                    SohImGui::GetConsole()->SendErrorMessage("Better Farore's Wind isn't turned on!");
                    return CMD_FAILED;
                }
                break;
        }
    }
    else {
        SohImGui::GetConsole()->SendErrorMessage("gPlayState == nullptr");
        return CMD_FAILED;
    }
    
    return CMD_SUCCESS;
}

static bool FileSelectHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (gPlayState != nullptr) {
        SET_NEXT_GAMESTATE(&gPlayState->state, FileChoose_Init, FileChooseContext);
        gPlayState->state.running = 0;
    } else {
        SohImGui::GetConsole()->SendErrorMessage("gPlayState == nullptr");
        return CMD_FAILED;
    }
    return CMD_SUCCESS;
}

static bool QuitHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    Ship::Window::GetInstance()->Close();
    return CMD_SUCCESS;
}

static bool SaveStateHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    unsigned int slot = OTRGlobals::Instance->gSaveStateMgr->GetCurrentSlot();
    const SaveStateReturn rtn = OTRGlobals::Instance->gSaveStateMgr->AddRequest({ slot, RequestType::SAVE });

    switch (rtn) {
        case SaveStateReturn::SUCCESS:
            SohImGui::GetConsole()->SendInfoMessage("[SOH] Saved state to slot %u", slot);
            return CMD_SUCCESS;
        case SaveStateReturn::FAIL_WRONG_GAMESTATE:
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Can not save a state outside of \"GamePlay\"");
            return CMD_FAILED;
    }
}

static bool LoadStateHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    unsigned int slot = OTRGlobals::Instance->gSaveStateMgr->GetCurrentSlot();
    const SaveStateReturn rtn = OTRGlobals::Instance->gSaveStateMgr->AddRequest({ slot, RequestType::LOAD });

    switch (rtn) {
        case SaveStateReturn::SUCCESS:
            SohImGui::GetConsole()->SendInfoMessage("[SOH] Loaded state from slot (%u)", slot);
            return CMD_SUCCESS;
        case SaveStateReturn::FAIL_INVALID_SLOT:
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Invalid State Slot Number (%u)", slot);
            return CMD_FAILED;
        case SaveStateReturn::FAIL_STATE_EMPTY:
            SohImGui::GetConsole()->SendErrorMessage("[SOH] State Slot (%u) is empty", slot);
            return CMD_FAILED;
        case SaveStateReturn::FAIL_WRONG_GAMESTATE:
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Can not load a state outside of \"GamePlay\"");
            return CMD_FAILED;
    }

}

static bool StateSlotSelectHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t slot;

    try {
        slot = std::stoi(args[1], nullptr, 10);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] SaveState slot value must be a number.");
        return CMD_FAILED;
    }

    if (slot < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Invalid slot passed. Slot must be between 0 and 2");
        return CMD_FAILED;
    }

    OTRGlobals::Instance->gSaveStateMgr->SetCurrentSlot(slot);
    SohImGui::GetConsole()->SendInfoMessage("[SOH] Slot %u selected",
                                        OTRGlobals::Instance->gSaveStateMgr->GetCurrentSlot());
    return CMD_SUCCESS;
}

static bool InvisibleHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Invisible value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::InvisibleLink();
    GameInteractionEffectQueryResult result = 
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Invisible Link %s", state ? "enabled" : "disabled");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not %s Invisible Link.",
                                                state ? "enable" : "disable");
        return CMD_FAILED;
    }
}

static bool GiantLinkHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Giant value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyLinkSize();
    effect->parameters[0] = GI_LINK_SIZE_GIANT;
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Giant Link %s", state ? "enabled" : "disabled");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not %s Giant Link.",
                                                state ? "enable" : "disable");
        return CMD_FAILED;
    }
}

static bool MinishLinkHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Minish value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyLinkSize();
    effect->parameters[0] = GI_LINK_SIZE_MINISH;
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Minish Link %s", state ? "enabled" : "disabled");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not %s Minish Link.",
                                                state ? "enable" : "disable");
        return CMD_FAILED;
    }
}

static bool AddHeartContainerHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    int hearts;

    try {
        hearts = std::stoi(args[1]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Hearts value must be an integer.");
        return CMD_FAILED;
    }

    if (hearts < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Hearts value must be a positive integer");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyHeartContainers();
    effect->parameters[0] = hearts;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Added %d heart containers", hearts);
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not add heart containers.");
        return CMD_FAILED;
    }
}

static bool RemoveHeartContainerHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    int hearts;

    try {
        hearts = std::stoi(args[1]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Hearts value must be an integer.");
        return CMD_FAILED;
    }

    if (hearts < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Hearts value must be a positive integer");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyHeartContainers();
    effect->parameters[0] = -hearts;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Removed %d heart containers", hearts);
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not remove heart containers.");
        return CMD_FAILED;
    }
}

static bool GravityHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyGravity();

    try {
        effect->parameters[0] = Ship::Math::clamp(std::stoi(args[1], nullptr, 10), GI_GRAVITY_LEVEL_LIGHT, GI_GRAVITY_LEVEL_HEAVY);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Gravity value must be a number.");
        return CMD_FAILED;
    }
    
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Updated gravity.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not update gravity.");
        return CMD_FAILED;
    }
}

static bool NoUIHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] No UI value must be a number.");
        return CMD_FAILED;
    }
    
    GameInteractionEffectBase* effect = new GameInteractionEffect::NoUI();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] No UI %s", state ? "enabled" : "disabled");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not %s No UI.",
                                                state ? "enable" : "disable");
        return CMD_FAILED;
    }
}

static bool FreezeHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::FreezePlayer();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Player frozen");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not freeze player.");
        return CMD_FAILED;
    }
}

static bool DefenseModifierHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyDefenseModifier();

    try {
        effect->parameters[0] = std::stoi(args[1], nullptr, 10);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Defense modifier value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Defense modifier set to %d", effect->parameters[0]);
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not set defense modifier.");
        return CMD_FAILED;
    }
}

static bool DamageHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyHealth();

    try {
        int value = std::stoi(args[1], nullptr, 10);
        if (value < 0) {
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Invalid value passed. Value must be greater than 0");
            return CMD_FAILED;
        }

        effect->parameters[0] = -value;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Damage value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Player damaged");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not damage player.");
        return CMD_FAILED;
    }
}

static bool HealHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyHealth();

    try {
        int value = std::stoi(args[1], nullptr, 10);
        if (value < 0) {
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Invalid value passed. Value must be greater than 0");
            return CMD_FAILED;
        }

        effect->parameters[0] = value;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Damage value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Player healed");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not heal player.");
        return CMD_FAILED;
    }
}

static bool FillMagicHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::FillMagic();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Magic filled");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not fill magic.");
        return CMD_FAILED;
    }
}

static bool EmptyMagicHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::EmptyMagic();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Magic emptied");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not empty magic.");
        return CMD_FAILED;
    }
}

static bool NoZHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
     if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
     uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] NoZ value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::DisableZTargeting();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] NoZ " + std::string(state ? "enabled" : "disabled"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not " +
                                                std::string(state ? "enable" : "disable") + " NoZ.");
        return CMD_FAILED;
    }
}

static bool OneHitKOHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] One-hit KO value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::OneHitKO();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] One-hit KO " + std::string(state ? "enabled" : "disabled"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not " +
                                                std::string(state ? "enable" : "disable") + " One-hit KO.");
        return CMD_FAILED;
    }
}

static bool PacifistHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Pacifist value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::PacifistMode();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Pacifist " + std::string(state ? "enabled" : "disabled"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not " +
                                                std::string(state ? "enable" : "disable") + " Pacifist.");
        return CMD_FAILED;
    }
}

static bool PaperLinkHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Paper Link value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyLinkSize();
    effect->parameters[0] = GI_LINK_SIZE_PAPER;
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Paper Link " + std::string(state ? "enabled" : "disabled"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not " +
                                                std::string(state ? "enable" : "disable") + " Paper Link.");
        return CMD_FAILED;
    }
}

static bool RainstormHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Rainstorm value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::WeatherRainstorm();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Rainstorm " + std::string(state ? "enabled" : "disabled"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not " +
                                                std::string(state ? "enable" : "disable") + " Rainstorm.");
        return CMD_FAILED;
    }
}

static bool ReverseControlsHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Reverse controls value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ReverseControls();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Reverse controls " +
                                                std::string(state ? "enabled" : "disabled"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not " +
                                                std::string(state ? "enable" : "disable") + " Reverse controls.");
        return CMD_FAILED;
    }
}

static bool UpdateRupeesHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyRupees();

    try {
        effect->parameters[0] = std::stoi(args[1], nullptr, 10);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Rupee value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Rupees updated");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not update rupees.");
        return CMD_FAILED;
    }
}

static bool SpeedModifierHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyRunSpeedModifier();

    try {
        effect->parameters[0] = std::stoi(args[1], nullptr, 10);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Speed modifier value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Speed modifier updated");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not update speed modifier.");
        return CMD_FAILED;
    }
}

const static std::map<std::string, uint16_t> boots {
    { "kokiri", PLAYER_BOOTS_KOKIRI },
    { "iron", PLAYER_BOOTS_IRON },
    { "hover", PLAYER_BOOTS_HOVER },
};

static bool BootsHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    const auto& it = boots.find(args[1]);
    if (it == boots.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Invalid boot type. Options are 'kokiri', 'iron' and 'hover'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ForceEquipBoots();
    effect->parameters[0] = it->second;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Boots updated.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not update boots.");
        return CMD_FAILED;
    }
}

const static std::map<std::string, uint16_t> shields {
    { "deku", ITEM_SHIELD_DEKU },
    { "hylian", ITEM_SHIELD_HYLIAN },
    { "mirror", ITEM_SHIELD_MIRROR },
};

static bool GiveShieldHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    const auto& it = shields.find(args[1]);
    if (it == shields.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Invalid shield type. Options are 'deku', 'hylian' and 'mirror'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::GiveOrTakeShield();
    effect->parameters[0] = it->second;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Gave shield.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not give shield.");
        return CMD_FAILED;
    }
}

static bool TakeShieldHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    const auto& it = shields.find(args[1]);
    if (it == shields.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Invalid shield type. Options are 'deku', 'hylian' and 'mirror'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::GiveOrTakeShield();
    effect->parameters[0] = it->second * -1;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Took shield.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not take shield.");
        return CMD_FAILED;
    }
}

static bool KnockbackHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::KnockbackPlayer();

    try {
        int value = std::stoi(args[1], nullptr, 10);
        if (value < 0) {
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Invalid value passed. Value must be greater than 0");
            return CMD_FAILED;
        }

        effect->parameters[0] = value;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Knockback value must be a number.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Knockback applied");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not apply knockback.");
        return CMD_FAILED;
    }
}

static bool ElectrocuteHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::ElectrocutePlayer();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Electrocuted player");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not electrocute player.");
        return CMD_FAILED;
    }
}

static bool BurnHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::BurnPlayer();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Burned player");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not burn player.");
        return CMD_FAILED;
    }
}

static bool CuccoStormHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectQueryResult result = GameInteractor::RawAction::SpawnActor(ACTOR_EN_NIW, 0);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Spawned cucco storm");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Command failed: Could not spawn cucco storm.");
        return CMD_FAILED;
    }
}

static bool GenerateRandoHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() == 1) {
        if (GenerateRandomizer()) {
            return CMD_SUCCESS;
        }
    }

    try {
        uint32_t value = std::stoi(args[1], NULL, 10);
        std::string seed = "";
        if (args.size() == 3) {
            int testing = std::stoi(args[1], nullptr, 10);
            seed = "seed_testing_count";
        }

        if (GenerateRandomizer(seed + std::to_string(value))){
            return CMD_SUCCESS;
        }
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] seed|count value must be a number.");
        return CMD_FAILED;
    }


    SohImGui::GetConsole()->SendErrorMessage("[SOH] Rando generation already in progress");
    return CMD_FAILED;
}

static bool CosmeticsHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    if (args[1].compare("reset") == 0) {
        CosmeticsEditor_ResetAll();
    } else if (args[1].compare("randomize") == 0) {
        CosmeticsEditor_RandomizeAll();
    } else {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Invalid argument passed, must be 'reset' or 'randomize'");
        return CMD_FAILED;
    }

    return CMD_SUCCESS;
}

static bool SfxHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Unexpected arguments passed");
        return CMD_FAILED;
    }

    if (args[1].compare("reset") == 0) {
        AudioEditor_ResetAll();
    } else if (args[1].compare("randomize") == 0) {
        AudioEditor_RandomizeAll();
    } else {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Invalid argument passed, must be 'reset' or 'randomize'");
        return CMD_FAILED;
    }

    return CMD_SUCCESS;
}

#define VARTYPE_INTEGER 0
#define VARTYPE_FLOAT   1
#define VARTYPE_STRING  2
#define VARTYPE_RGBA    3

static int CheckVarType(const std::string& input)
{
    int result = VARTYPE_STRING;

    if (input[0] == '#') {
        return VARTYPE_RGBA;
    }

    for (size_t i = 0; i < input.size(); i++)
    {
        if (!(std::isdigit(input[i]) || input[i] == '.'))
        {
            break;
        }
        else
        {
            if (input[i] == '.')
                result = VARTYPE_FLOAT;
            else if (std::isdigit(input[i]) && result != VARTYPE_FLOAT)
                result = VARTYPE_INTEGER;
        }
    }

    return result;
}

static bool SetCVarHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 3)
        return CMD_FAILED;

    int vType = CheckVarType(args[2]);

    if (vType == VARTYPE_STRING)
        CVarSetString(args[1].c_str(), args[2].c_str());
    else if (vType == VARTYPE_FLOAT)
        CVarSetFloat((char*)args[1].c_str(), std::stof(args[2]));
    else if (vType == VARTYPE_RGBA)
    {
        uint32_t val = std::stoul(&args[2].c_str()[1], nullptr, 16);
        Color_RGBA8 clr;
        clr.r = val >> 24;
        clr.g = val >> 16;
        clr.b = val >> 8;
        clr.a = val & 0xFF;
        CVarSetColor((char*)args[1].c_str(), clr);
    }
    else
        CVarSetInteger(args[1].c_str(), std::stoi(args[2]));

    CVarSave();

    //SohImGui::GetConsole()->SendInfoMessage("[SOH] Updated player position to [ %.2f, %.2f, %.2f ]", pos.x, pos.y, pos.z);
    return CMD_SUCCESS;
}

static bool GetCVarHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2)
        return CMD_FAILED;

    auto cvar = CVarGet(args[1].c_str());

    if (cvar != nullptr)
    {
        if (cvar->Type == Ship::ConsoleVariableType::Integer)
            SohImGui::GetConsole()->SendInfoMessage("[SOH] Variable %s is %i", args[1].c_str(), cvar->Integer);
        else if (cvar->Type == Ship::ConsoleVariableType::Float)
            SohImGui::GetConsole()->SendInfoMessage("[SOH] Variable %s is %f", args[1].c_str(), cvar->Float);
        else if (cvar->Type == Ship::ConsoleVariableType::String)
            SohImGui::GetConsole()->SendInfoMessage("[SOH] Variable %s is %s", args[1].c_str(), cvar->String.c_str());
        else if (cvar->Type == Ship::ConsoleVariableType::Color)
            SohImGui::GetConsole()->SendInfoMessage("[SOH] Variable %s is %08X", args[1].c_str(), cvar->Color);
    }
    else
    {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Could not find variable %s", args[1].c_str());
    }


    return CMD_SUCCESS;
}

void DebugConsole_Init(void) {
    // Console
    CMD_REGISTER("file_select", { FileSelectHandler, "Returns to the file select." });
    CMD_REGISTER("reset", { ResetHandler, "Resets the game." });
    CMD_REGISTER("quit", { QuitHandler, "Quits the game." });

    // Save States
    CMD_REGISTER("save_state", { SaveStateHandler, "Save a state." });
    CMD_REGISTER("load_state", { LoadStateHandler, "Load a state." });
    CMD_REGISTER("set_slot", { StateSlotSelectHandler, "Selects a SaveState slot", {
        { "Slot number", Ship::ArgumentType::NUMBER, }
    }});

    // Map & Location
    CMD_REGISTER("void", { VoidHandler, "Voids out of the current map." });
    CMD_REGISTER("reload", { ReloadHandler, "Reloads the current map." });
    CMD_REGISTER("fw", { FWHandler,"Spawns the player where Farore's Wind is set." , {
        { "clear|warp|backup", Ship::ArgumentType::TEXT }
    }});
    CMD_REGISTER("entrance", { EntranceHandler, "Sends player to the entered entrance (hex)", {
        { "entrance", Ship::ArgumentType::NUMBER }
    }});

    // Gameplay
    CMD_REGISTER("kill", { KillPlayerHandler, "Commit suicide." });

    CMD_REGISTER("map",  { LoadSceneHandler, "Load up kak?" });

    CMD_REGISTER("rupee", { RupeeHandler, "Set your rupee counter.", {
        {"amount", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("bItem", { BHandler, "Set an item to the B button.", {
        { "Item ID", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("spawn", { ActorSpawnHandler, "Spawn an actor.", { { "actor_id", Ship::ArgumentType::NUMBER },
        { "data", Ship::ArgumentType::NUMBER },
        { "x", Ship::ArgumentType::PLAYER_POS, true },
        { "y", Ship::ArgumentType::PLAYER_POS, true },
        { "z", Ship::ArgumentType::PLAYER_POS, true },
        { "rx", Ship::ArgumentType::PLAYER_ROT, true },
        { "ry", Ship::ArgumentType::PLAYER_ROT, true },
        { "rz", Ship::ArgumentType::PLAYER_ROT, true }
    }});

    CMD_REGISTER("pos", { SetPosHandler, "Sets the position of the player.", {
        { "x", Ship::ArgumentType::PLAYER_POS, true },
        { "y", Ship::ArgumentType::PLAYER_POS, true },
        { "z", Ship::ArgumentType::PLAYER_POS, true }
    }});

    CMD_REGISTER("set", { SetCVarHandler,  "Sets a console variable.", {
        { "varName", Ship::ArgumentType::TEXT },
        { "varValue", Ship::ArgumentType::TEXT }
    }});

    CMD_REGISTER("get", { GetCVarHandler, "Gets a console variable.", {
        { "varName", Ship::ArgumentType::TEXT }
    }});
    
    CMD_REGISTER("addammo", { AddAmmoHandler, "Adds ammo of an item.", {
        { "sticks|nuts|bombs|seeds|arrows|bombchus|beans", Ship::ArgumentType::TEXT },
        { "count", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("takeammo", { TakeAmmoHandler, "Removes ammo of an item.", {
        { "sticks|nuts|bombs|seeds|arrows|bombchus|beans", Ship::ArgumentType::TEXT },
        { "count", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("bottle", { BottleHandler, "Changes item in a bottle slot.", {
        { "item", Ship::ArgumentType::TEXT },
        { "slot", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("give_item", { GiveItemHandler,  "Gives an item to the player as if it was given from an actor", {
        { "vanilla|randomizer", Ship::ArgumentType::TEXT },
        { "giveItemID", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("item", { ItemHandler,  "Sets item ID in arg 1 into slot arg 2. No boundary checks. Use with caution.", {
        { "slot", Ship::ArgumentType::NUMBER },
        { "item id", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("invisible", { InvisibleHandler, "Activate Link's Elvish cloak, making him appear invisible.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("giant_link", { GiantLinkHandler, "Turn Link into a giant Lonky boi.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("minish_link", { MinishLinkHandler, "Turn Link into a minish boi.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("add_heart_container", { AddHeartContainerHandler, "Give Link a heart! The maximum amount of hearts is 20!" });

    CMD_REGISTER("remove_heart_container", { RemoveHeartContainerHandler, "Remove a heart from Link. The minimal amount of hearts is 3." });

    CMD_REGISTER("gravity", { GravityHandler, "Set gravity level.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("no_ui", { NoUIHandler, "Disables the UI.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("freeze", { FreezeHandler, "Freezes Link in place" });

    CMD_REGISTER("defense_modifier", { DefenseModifierHandler, "Sets the defense modifier.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("damage", { DamageHandler, "Deal damage to Link.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("heal", { HealHandler, "Heals Link.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("fill_magic", { FillMagicHandler, "Fills magic." });

    CMD_REGISTER("empty_magic", { EmptyMagicHandler, "Empties magic." });

    CMD_REGISTER("no_z", { NoZHandler, "Disables Z-button presses.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("ohko", { OneHitKOHandler, "Activates one hit KO. Any damage kills Link and he cannot gain health in this mode.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("pacifist", { PacifistHandler, "Activates pacifist mode. Prevents Link from using his weapon.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("paper_link", { PaperLinkHandler, "Link but made out of paper.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("rainstorm", { RainstormHandler, "Activates rainstorm." });

    CMD_REGISTER("reverse_controls", { ReverseControlsHandler, "Reverses the controls.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("update_rupees", { UpdateRupeesHandler, "Adds rupees.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("speed_modifier", { SpeedModifierHandler, "Sets the speed modifier.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("boots", { BootsHandler, "Activates boots.", {
        { "kokiri|iron|hover", Ship::ArgumentType::TEXT },
    }});

    CMD_REGISTER("giveshield", { GiveShieldHandler, "Gives a shield and equips it when Link is the right age for it.", {
        { "deku|hylian|mirror", Ship::ArgumentType::TEXT },
    }});

    CMD_REGISTER("takeshield", { TakeShieldHandler, "Takes a shield and unequips it if Link is wearing it.", {
        { "deku|hylian|mirror", Ship::ArgumentType::TEXT },
    }});

    CMD_REGISTER("knockback", { KnockbackHandler, "Knocks Link back.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("electrocute", { ElectrocuteHandler, "Electrocutes Link." });

    CMD_REGISTER("burn", { BurnHandler, "Burns Link." });

    CMD_REGISTER("cucco_storm", { CuccoStormHandler, "Cucco Storm" });

    CMD_REGISTER("gen_rando", { GenerateRandoHandler, "Generate a randomizer seed", {
        { "seed|count", Ship::ArgumentType::NUMBER, true },
        { "testing", Ship::ArgumentType::NUMBER, true },
    }});

    CMD_REGISTER("cosmetics", { CosmeticsHandler, "Change cosmetics.", {
        { "reset|randomize", Ship::ArgumentType::TEXT },
    }});

    CMD_REGISTER("sfx", { SfxHandler, "Change SFX.", {
        { "reset|randomize", Ship::ArgumentType::TEXT },
    }});

    CVarLoad();
}
