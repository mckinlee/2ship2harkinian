#include "2s2h/CustomMessage/CustomMessage.h"
#include "MiscBehavior.h"
#include "2s2h/ShipUtils.h"
#include <set>
#include "2s2h/Rando/Logic/Logic.h"

extern "C" {
#include <variables.h>
#include <z64ocarina.h>
s32 Message_ShouldAdvanceSilent(PlayState* play);
}

static int playedSariasSongState = 0;

std::set<RandoItemId> progressiveItems = {
    RI_BOW,       RI_HOOKSHOT,          RI_MASK_BLAST,  RI_BOMB_BAG_20,  RI_MASK_DEKU, RI_MASK_GORON,
    RI_MASK_ZORA, RI_MASK_FIERCE_DEITY, RI_SONG_SONATA, RI_SONG_LULLABY, RI_SONG_NOVA, RI_SONG_SOARING,
};

RandoCheckId GetProgressiveCheckInLogic() {
    std::set<RandoRegionId> reachableRegions = {};
    // Get connected entrances from starting & warp points
    Rando::Logic::FindReachableRegions(RR_MAX, reachableRegions);
    // Get connected regions from current entrance (TODO: Make this optional)
    Rando::Logic::FindReachableRegions(Rando::Logic::GetRegionIdFromEntrance(gSaveContext.save.entrance),
                                       reachableRegions);

    std::vector<RandoCheckId> progressiveChecks = {};

    for (RandoRegionId regionId : reachableRegions) {
        auto& randoRegion = Rando::Logic::Regions[regionId];
        for (auto& [randoCheckId, accessLogicFunc] : randoRegion.checks) {
            if (accessLogicFunc.first() && RANDO_SAVE_CHECKS[randoCheckId].shuffled &&
                !RANDO_SAVE_CHECKS[randoCheckId].obtained) {
                RandoItemId itemId = RANDO_SAVE_CHECKS[randoCheckId].randoItemId;

                if (progressiveItems.count(Rando::ConvertItem(itemId, randoCheckId))) {
                    progressiveChecks.push_back(randoCheckId);
                }
            }
        }
    }

    return progressiveChecks.empty() ? RC_UNKNOWN : progressiveChecks[0];
}

void Rando::MiscBehavior::SariasSongHint() {
    bool shouldRegister = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_SARIAS_SONG];

    COND_VB_SHOULD(VB_SONG_AVAILABLE_TO_PLAY, shouldRegister, {
        uint8_t* songIndex = va_arg(args, uint8_t*);
        // If the currently played song is Sun's Song, set it to be available to be played.
        if (*songIndex == OCARINA_SONG_SARIAS && CHECK_QUEST_ITEM(QUEST_SONG_SARIA)) {
            *should = true;
            playedSariasSongState = 1;
        }
    });

    COND_VB_SHOULD(VB_MSGMODE_TEXT_DONE_CAPTURE_DEBUG_END, shouldRegister, {
        if (*should && playedSariasSongState &&
            gPlayState->msgCtx.ocarinaMode == OCARINA_MODE_PROCESS_RESTRICTED_SONG) {
            if (Message_ShouldAdvanceSilent(gPlayState)) {
                if (gPlayState->msgCtx.choiceIndex == 0) {
                    playedSariasSongState = 2;
                    Audio_PlaySfx(NA_SE_SY_DECIDE);
                    Message_ContinueTextbox(gPlayState, 0x1B95);
                } else {
                    playedSariasSongState = 0;
                    Audio_PlaySfx(NA_SE_SY_DECIDE);
                    Message_CloseTextbox(gPlayState);
                    gPlayState->msgCtx.ocarinaMode = OCARINA_MODE_END;
                }
            }
        }
    });

    COND_ID_HOOK(OnOpenText, 0x1B95, shouldRegister, [](u16* textId, bool* loadFromMessageTable) {
        CustomMessage::Entry entry;
        if (playedSariasSongState == 1) {
            entry.nextMessageID = 0x1B95;
            entry.msg = "Call out to an old friend for help? You can only do this once.\x02\x11\xC2Yes\x11No";
        } else if (playedSariasSongState == 2) {
            RandoCheckId randoCheckId = GetProgressiveCheckInLogic();
            entry.textboxType = TEXTBOX_TYPE_2;
            entry.autoFormat = false;

            if (randoCheckId == RC_UNKNOWN) {
                entry.msg = "\x18%g.\x1F%w\x10.\x1F%w\x10.\x1F%w\x10 You call out but there is no response...";
                CustomMessage::EnsureMessageEnd(&entry.msg);
                CustomMessage::ReplaceColorChars(&entry.msg);
            } else {
                entry.msg =
                    "\x18%g.\x1F%w\x10.\x1F%w\x10.\x1F%w\x10 Link?\x1F%w\x10 Is that you?\x1F%w\x10 Where have you\x11"
                    "been..?!\x1F%w\x10 Zelda has been worried sick\x11"
                    "about you!\x1F%w\x10 .\x1F%w\x10.\x1F%w\x10.\x1F%w\x10 You need my help?\x1F%w\x10 \x10 ... "
                    "Alright but just this once.\x1F%w\x10 Search\x11%y{{location}}%g, you will find\x11what you need.";
                CustomMessage::Replace(&entry.msg, "{{location}}",
                                       Ship_GetSceneName(Rando::StaticData::Checks[randoCheckId].sceneId));
                CustomMessage::EnsureMessageEnd(&entry.msg);
                CustomMessage::ReplaceColorChars(&entry.msg);
                REMOVE_QUEST_ITEM(QUEST_SONG_SARIA);
            }

            playedSariasSongState = 0;
        }

        CustomMessage::LoadCustomMessageIntoFont(entry);
        *loadFromMessageTable = false;
    });
}
