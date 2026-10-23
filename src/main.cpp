// Adaptive early-input buffer for Geometry Dash 2.2081 / Geode 5.11.2.
// Only a hidden player is simulated. Real collisions and deaths stay enabled.
#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/ui/Notification.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>

using namespace geode::prelude;

namespace pib {
    using Clock = std::chrono::steady_clock;
    struct Predictor;
    inline Predictor* active = nullptr;
    enum class Verdict { Clear, Collision, Unknown };
    struct Event { uint64_t tick; bool down; };

    int limit() {
        return std::clamp(static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-delay")), 0, 16);
    }

    // Geode exposes renamed fields as anonymous unions, making GJGameState
    // non-copyable. Copy canonical fields individually, including containers;
    // never memcpy owning containers or duplicate their allocation pointers.
#define PIB_STATE_FIELDS(F) \
        F(m_cameraZoom) \
        F(m_targetCameraZoom) \
        F(m_cameraOffset) \
        F(m_unkPoint1) \
        F(m_unkPoint2) \
        F(m_unkPoint3) \
        F(m_cameraDeltaPosition) \
        F(m_unkPoint5) \
        F(m_unkPoint6) \
        F(m_unkPoint7) \
        F(m_unkPoint8) \
        F(m_unkPoint9) \
        F(m_unkPoint10) \
        F(m_unkPoint11) \
        F(m_unkPoint12) \
        F(m_unkPoint13) \
        F(m_unkPoint14) \
        F(m_unkPoint15) \
        F(m_unkPoint16) \
        F(m_unkPoint17) \
        F(m_unkPoint18) \
        F(m_unkPoint19) \
        F(m_unkPoint20) \
        F(m_unkPoint21) \
        F(m_unkPoint22) \
        F(m_unkPoint23) \
        F(m_unkPoint24) \
        F(m_unkPoint25) \
        F(m_unkPoint26) \
        F(m_unkPoint27) \
        F(m_unkPoint28) \
        F(m_unkPoint29) \
        F(m_unkBool1) \
        F(m_unkInt1) \
        F(m_unkBool2) \
        F(m_unkInt2) \
        F(m_unkBool3) \
        F(m_unkPoint30) \
        F(m_middleGroundOffsetY) \
        F(m_unkInt3) \
        F(m_unkInt4) \
        F(m_unkBool4) \
        F(m_unkBool5) \
        F(m_cameraPadding) \
        F(m_cameraEasing) \
        F(m_unkInt5) \
        F(m_unkInt6) \
        F(m_unkInt7) \
        F(m_unkInt8) \
        F(m_unkInt9) \
        F(m_unkInt10) \
        F(m_lastModeHeight) \
        F(m_lastCameraZoom) \
        F(m_unkUint1) \
        F(m_portalY) \
        F(m_unkBool6) \
        F(m_gravityRelated) \
        F(m_unkInt12) \
        F(m_unkInt13) \
        F(m_unkInt14) \
        F(m_unkInt15) \
        F(m_unkBool7) \
        F(m_isFreeMode) \
        F(m_disableCameraGridSnap) \
        F(m_backgroundSpeedX) \
        F(m_backgroundSpeedY) \
        F(m_middlegroundSpeedX) \
        F(m_middlegroundSpeedY) \
        F(m_cameraAngle) \
        F(m_targetCameraAngle) \
        F(m_playerStreakBlend) \
        F(m_timeWarp) \
        F(m_queuedTimeWarp) \
        F(m_timeWarpRelated) \
        F(m_currentChannel) \
        F(m_rotateChannel) \
        F(m_spawnChannelRelated0) \
        F(m_spawnChannelRelated1) \
        F(m_totalTime) \
        F(m_levelTime) \
        F(m_unkDouble3) \
        F(m_commandIndex) \
        F(m_unkUint3) \
        F(m_currentProgress) \
        F(m_unkUint4) \
        F(m_unkUint5) \
        F(m_unkUint6) \
        F(m_unkUint7) \
        F(m_lastActivatedPortal1) \
        F(m_lastActivatedPortal2) \
        F(m_cameraPosition) \
        F(m_isFlipped) \
        F(m_levelFlipProgress) \
        F(m_inSecondHalfOfFlipAnimation) \
        F(m_isFlipping) \
        F(m_isDualMode) \
        F(m_dualAnimationProgress) \
        F(m_tweenActions) \
        F(m_cameraEdgeValue0) \
        F(m_cameraEdgeValue1) \
        F(m_cameraEdgeValue2) \
        F(m_cameraEdgeValue3) \
        F(m_gameObjectPhysics) \
        F(m_unkVecFloat1) \
        F(m_unkUint10) \
        F(m_unkUint11) \
        F(m_unkUint12) \
        F(m_cameraStepDiff) \
        F(m_backgroundWidth) \
        F(m_timeModRelated) \
        F(m_timeModRelated2) \
        F(m_activatedObjectIDs) \
        F(m_unkUint13) \
        F(m_unkPoint32) \
        F(m_cameraLastPosition) \
        F(m_unkBool20) \
        F(m_unkBool21) \
        F(m_audioOnDeath) \
        F(m_respawnTime) \
        F(m_noDeathSFX) \
        F(m_cameraShakeEnabled) \
        F(m_cameraShakeDuration) \
        F(m_cameraShakeStrength) \
        F(m_cameraShakeInterval) \
        F(m_lastShakeTime) \
        F(m_unkPoint34) \
        F(m_dualRelated) \
        F(m_stateObjects) \
        F(m_unkMapPairGJGameEventIntVectorEventTriggerInstance) \
        F(m_unkMapPairGJGameEventIntInt) \
        F(m_enterEffectInstanceVectors) \
        F(m_exitEffectInstanceVectors) \
        F(m_enterChannelMap) \
        F(m_exitChannelMap) \
        F(m_moveEffectInstances) \
        F(m_rotateEffectInstances) \
        F(m_scaleEffectInstances) \
        F(m_fadeEffectInstances) \
        F(m_tintEffectInstances) \
        F(m_unsortedAreaEffects) \
        F(m_unkBool27) \
        F(m_advanceFollowInstances) \
        F(m_dynamicMoveActions) \
        F(m_dynamicRotateActions) \
        F(m_instantExitStaticCameraX) \
        F(m_instantExitStaticCameraY) \
        F(m_unkUint17) \
        F(m_unkUMap8) \
        F(m_proximityVolumeRelated) \
        F(m_songChannelStates) \
        F(m_songTriggerStateVectors) \
        F(m_sfxTriggerStates) \
        F(m_unkBool30) \
        F(m_background) \
        F(m_ground) \
        F(m_middleground) \
        F(m_unlinkDualGravity) \
        F(m_points) \
        F(m_hasObtainedPoints) \
        F(m_pauseCounter) \
        F(m_pauseBufferTimer)
    struct GameStateSnapshot {
#define PIB_DECLARE(name) decltype(GJGameState::name) name;
        PIB_STATE_FIELDS(PIB_DECLARE)
#undef PIB_DECLARE
        explicit GameStateSnapshot(GJGameState const& source) {
#define PIB_CAPTURE(name) name = source.name;
            PIB_STATE_FIELDS(PIB_CAPTURE)
#undef PIB_CAPTURE
        }
        void restore(GJGameState& target) const {
#define PIB_RESTORE(name) target.name = name;
            PIB_STATE_FIELDS(PIB_RESTORE)
#undef PIB_RESTORE
        }
    };
#undef PIB_STATE_FIELDS
    struct Predictor {
        PlayLayer* owner;
        Ref<PlayerObject> ghost;
        Ref<PlayerCheckpoint> checkpoint;
        std::deque<Event> pending;
        uint64_t tick = 0;
        int shift = 0;
        float step = 0.25f; // PlayerObject uses GD units: one 240 Hz step = 0.25.
        bool physicalDown = false;
        bool injecting = false;
        bool simulating = false;
        bool collision = false;
        bool uncertain = false;
        Clock::time_point deadline;

        explicit Predictor(PlayLayer* layer) : owner(layer) {}
        ~Predictor() { if (active == this) active = nullptr; }

        void clear() {
            pending.clear();
            shift = 0;
            tick = 0;
            physicalDown = false;
        }

        bool supported() const {
            auto* p = owner->m_player1;
            return p && !p->m_isDead && !owner->m_isPaused &&
                !owner->m_isPlatformer && !owner->m_gameState.m_isDualMode &&
                !p->m_isSpider && !p->m_isDashing;
        }

        bool prepare() {
            if (!ghost) {
                // Use the actual layer pointer; PlayLayer::get() is not reliable in init.
                ghost = PlayerObject::create(1, 1, owner, owner, true);
                if (!ghost) return false;
                owner->m_objectLayer->addChild(ghost.data());
                ghost->setVisible(false);
            }
            if (!checkpoint) checkpoint = PlayerCheckpoint::create();
            if (!checkpoint) return false;
            owner->m_player1->saveToCheckpoint(checkpoint.data());
            return true;
        }

        struct ProbeScope {
            Predictor& predictor;
            GameStateSnapshot saved;
            explicit ProbeScope(Predictor& p) : predictor(p), saved(p.owner->m_gameState) {
                p.simulating = true;
                p.collision = false;
                p.uncertain = false;
            }
            ~ProbeScope() {
                saved.restore(predictor.owner->m_gameState);
                predictor.simulating = false;
            }
        };

        Verdict probe(bool down, int delay, int horizon) {
            ProbeScope scope(*this);
            auto* real = owner->m_player1;
            ghost->copyAttributes(real);
            ghost->loadFromCheckpoint(checkpoint.data());
            ghost->setScale(real->getScale());
            ghost->setContentSize(real->getContentSize());
            ghost->m_width = real->m_width;
            ghost->m_height = real->m_height;
            ghost->m_holdingButtons = real->m_holdingButtons;
            ghost->m_isDead = false;
            ghost->setVisible(false);

            // All candidates cover the SAME future interval. A later crash by
            // itself does not count as a successful correction.
            for (int i = 0; i < horizon; ++i) {
                if (Clock::now() >= deadline) return Verdict::Unknown;
                if (i == delay) {
                    if (down) ghost->pushButton(PlayerButton::Jump);
                    else ghost->releaseButton(PlayerButton::Jump);
                }
                ghost->update(step);
                ghost->updateRotation(step);
                if (uncertain) return Verdict::Unknown;
                if (collision || ghost->m_isDead) return Verdict::Collision;
                ghost->m_collisionLogTop->removeAllObjects();
                ghost->m_collisionLogBottom->removeAllObjects();
                ghost->m_collisionLogLeft->removeAllObjects();
                ghost->m_collisionLogRight->removeAllObjects();
                owner->checkCollisions(ghost.data(), step, false);
                if (uncertain) return Verdict::Unknown;
                if (collision || ghost->m_isDead) return Verdict::Collision;
            }
            return Verdict::Clear;
        }

        int chooseDelay(bool down) {
            const int maximum = limit();
            if (!maximum || !supported()) return 0;
            const double budget = Mod::get()->getSettingValue<double>("prediction-budget");
            deadline = Clock::now() + std::chrono::microseconds(static_cast<int>(budget * 1000));
            if (!prepare()) return 0;
            auto* p = owner->m_player1;
            // A cube jump needs enough lookahead to see its landing. A wave
            // must normally be steered again much sooner.
            const int horizon = static_cast<int>(Mod::get()->getSettingValue<int64_t>(
                p->m_isDart || p->m_isShip || p->m_isSwing ? "flight-horizon" : "ground-horizon"));
            if (probe(down, 0, horizon) != Verdict::Collision) return 0;
            for (int delay = 1; delay <= maximum; ++delay) {
                const auto verdict = probe(down, delay, horizon);
                if (verdict == Verdict::Clear) return delay;
                if (verdict == Verdict::Unknown) return 0;
            }
            return 0;
        }

        void fire(bool down) {
            injecting = true;
            owner->handleButton(down, 1, true);
            injecting = false;
        }

        void flush() {
            // Reconcile the held state when disabling the buffer or leaving a
            // supported mode, rather than replaying obsolete presses at once.
            const bool hadPending = !pending.empty();
            pending.clear();
            shift = 0;
            if (hadPending && owner->m_player1 && !owner->m_player1->m_isDead)
                fire(physicalDown);
        }

        bool intercept(bool down) {
            physicalDown = down;
            if (!supported() || !limit()) {
                pending.clear();
                shift = 0;
                return false;
            }
            if (!pending.empty()) {
                // Shift the complete tap by the original amount, so a release
                // cannot overtake its press and short taps keep their duration.
                pending.push_back({tick + static_cast<uint64_t>(shift), down});
                return true;
            }
            const int delay = chooseDelay(down);
            if (!delay) return false;
            shift = delay;
            pending.push_back({tick + static_cast<uint64_t>(delay), down});
            if (Mod::get()->getSettingValue<bool>("verbose-log")) {
                log::info("[PIB] {} delayed {} ticks (limit {}). Immediate path collided; delayed path cleared prediction.",
                    down ? "PRESS" : "RELEASE", delay, limit());
            }
            return true;
        }

        void beforeStep(float dt) {
            // Keep time-warp and sub-tick steps in the same units as the game.
            if (dt > 0.f && dt <= 1.f) step = dt;
            if (!supported() || !limit()) { flush(); return; }
            while (!pending.empty() && pending.front().tick <= tick) {
                const bool down = pending.front().down;
                pending.pop_front();
                fire(down);
            }
            if (pending.empty()) shift = 0;
        }
    };

    bool simulating() { return active && active->simulating; }

    bool solid(GameObjectType type) {
        return type == GameObjectType::Solid || type == GameObjectType::Slope ||
            type == GameObjectType::Hazard || type == GameObjectType::AnimatedHazard;
    }

    bool inert(GameObjectType type) {
        return type == GameObjectType::Decoration || type == GameObjectType::SecretCoin ||
            type == GameObjectType::UserCoin || type == GameObjectType::Collectible;
    }

    void cycle() {
        constexpr int stages[] = {0, 1, 2, 4, 8, 16};
        int next = 0;
        for (int value : stages) if (value > limit()) { next = value; break; }
        if (active) active->flush();
        Mod::get()->setSettingValue<int64_t>("max-delay", next);
        const auto message = next ? fmt::format("Input Buffer: hasta {} ticks", next) : "Input Buffer: OFF";
        Notification::create(message, NotificationIcon::Info, 1.5f)->show();
        log::info("[PIB] {}", message);
    }
}

class $modify(AdaptivePlayLayer, PlayLayer) {
    struct Fields { std::unique_ptr<pib::Predictor> predictor; };

    static void onModify(auto& self) {
        (void)self.setHookPriority("PlayLayer::destroyPlayer", -1000000);
        (void)self.setHookPriority("PlayLayer::playEndAnimationToPos", -1000000);
    }

    bool init(GJGameLevel* level, bool replay, bool skipObjects) {
        if (!PlayLayer::init(level, replay, skipObjects)) return false;
        m_fields->predictor = std::make_unique<pib::Predictor>(this);
        pib::active = m_fields->predictor.get();
        return true;
    }

    void resetLevel() {
        if (m_fields->predictor) m_fields->predictor->clear();
        PlayLayer::resetLevel();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (pib::simulating() && player == pib::active->ghost.data()) {
            pib::active->collision = true;
            return;
        }
        if (m_fields->predictor) m_fields->predictor->clear();
        PlayLayer::destroyPlayer(player, object);
    }

    void playEndAnimationToPos(CCPoint position) {
        if (pib::simulating()) { pib::active->uncertain = true; return; }
        PlayLayer::playEndAnimationToPos(position);
    }

    void onQuit() {
        if (pib::active == m_fields->predictor.get()) pib::active = nullptr;
        m_fields->predictor.reset();
        PlayLayer::onQuit();
    }
};

class $modify(AdaptiveBaseLayer, GJBaseGameLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("GJBaseGameLayer::canBeActivatedByPlayer", -1000000);
        (void)self.setHookPriority("GJBaseGameLayer::collisionCheckObjects", -1000000);
        (void)self.setHookPriority("GJBaseGameLayer::gameEventTriggered", -1000000);
        (void)self.setHookPriority("GJBaseGameLayer::playerTouchedRing", -1000000);
        (void)self.setHookPriority("GJBaseGameLayer::playerWillSwitchMode", -1000000);
        (void)self.setHookPriority("GJBaseGameLayer::flipGravity", -1000000);
    }
    void handleButton(bool down, int button, bool player1) {
        auto* p = pib::active;
        if (p && static_cast<GJBaseGameLayer*>(this) == static_cast<GJBaseGameLayer*>(p->owner) && !p->simulating && !p->injecting &&
            button == 1 && player1 && p->intercept(down)) return;
        GJBaseGameLayer::handleButton(down, button, player1);
    }

    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* objects, int count, float dt) {
        if (!pib::simulating()) {
            GJBaseGameLayer::collisionCheckObjects(player, objects, count, dt);
            return;
        }
        gd::vector<GameObject*> geometry;
        const auto bounds = player->getObjectRect();
        const int size = std::min(count, static_cast<int>(objects->size()));
        geometry.reserve(size);
        for (int i = 0; i < size; ++i) {
            auto* object = (*objects)[i];
            if (!object) continue;
            if (pib::solid(object->m_objectType)) geometry.push_back(object);
            else if (!pib::inert(object->m_objectType) && bounds.intersectsRect(object->getObjectRect())) {
                // Portals, pads, orbs and special objects need world simulation.
                // Return an unknown result so the real input passes unchanged.
                pib::active->uncertain = true;
            }
        }
        if (!pib::active->uncertain)
            GJBaseGameLayer::collisionCheckObjects(player, &geometry, static_cast<int>(geometry.size()), dt);
    }

    bool canBeActivatedByPlayer(PlayerObject* player, EffectGameObject* object) {
        if (pib::simulating()) return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(player, object);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (pib::simulating()) return;
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    void gameEventTriggered(GJGameEvent event, int material, int playerID) {
        if (pib::simulating()) return;
        GJBaseGameLayer::gameEventTriggered(event, material, playerID);
    }

    void flipGravity(PlayerObject* player, bool flip, bool noEffects) {
        if (pib::simulating()) { player->flipGravity(flip, true); return; }
        GJBaseGameLayer::flipGravity(player, flip, noEffects);
    }

    void playerWillSwitchMode(PlayerObject* player, GameObject* object) {
        if (pib::simulating()) { pib::active->uncertain = true; return; }
        GJBaseGameLayer::playerWillSwitchMode(player, object);
    }
};

class $modify(AdaptivePlayer, PlayerObject) {
    void update(float dt) {
        auto* p = pib::active;
        const bool realStep = p && !p->simulating && dt > 0.f && this == p->owner->m_player1;
        if (realStep) p->beforeStep(dt);
        PlayerObject::update(dt);
        // Increment AFTER the step: a one-tick delay must not fire before this
        // step's physics, which would be an accidental zero-tick delay.
        if (realStep) ++p->tick;
    }

    void incrementJumps() {
        if (!pib::simulating()) PlayerObject::incrementJumps();
    }

    void playSpiderDashEffect(CCPoint from, CCPoint to) {
        if (!pib::simulating()) PlayerObject::playSpiderDashEffect(from, to);
    }

    void ringJump(RingObject* ring, bool skipCheck) {
        if (!pib::simulating()) PlayerObject::ringJump(ring, skipCheck);
    }
};

class $modify(AdaptiveKeyboard, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (key == KEY_F6 && pib::active) {
            if (down && !repeat) pib::cycle();
            return true;
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};

class $modify(AdaptiveEffects, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* layer, int group, gd::vector<int> const* remap) {
        if (!pib::simulating()) EffectGameObject::triggerObject(layer, group, remap);
    }
};

class $modify(AdaptiveShine, GameObject) {
    void playShineEffect() {
        if (!pib::simulating()) GameObject::playShineEffect();
    }
};

class $modify(AdaptiveTrail, HardStreak) {
    void addPoint(CCPoint point) {
        if (!pib::simulating()) HardStreak::addPoint(point);
    }
};

$on_mod(Loaded) {
    log::info("[PIB] Adaptive early-input buffer loaded. F6: 1 / 2 / 4 / 8 / 16 / OFF. Inputs pass immediately unless a delayed trajectory clears a predicted collision.");
}
