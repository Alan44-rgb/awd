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
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/UILayer.hpp>
#include "buffer_core.hpp"
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
    using Verdict = core::Kind;
    struct Event { uint64_t tick; bool down; };
    void toggleMenu();

    bool enabled() { return Mod::get()->getSettingValue<bool>("enabled"); }

    int limit() {
        return core::ticks(static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-delay")));
    }

    int releaseLimit() {
        return core::ticks(static_cast<int>(Mod::get()->getSettingValue<int64_t>("release-ticks")));
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
        Ref<CCLabelBMFont> indicator;
        std::unique_ptr<GameStateSnapshot> baselineWorld;
        core::Decision lastDecision;
        GameObject* killedBy = nullptr;
        std::string status = "Listo";
        int corrections = 0;
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

        char const* mode() const {
            auto* p = owner->m_player1;
            if (!p) return "Menu";
            if (p->m_isDart) return "Wave";
            if (p->m_isShip) return "Ship";
            if (p->m_isBall) return "Ball";
            if (p->m_isBird) return "UFO";
            if (p->m_isRobot) return "Robot";
            if (p->m_isSpider) return "Spider";
            if (p->m_isSwing) return "Swing";
            return "Cube";
        }

        void refreshIndicator() {
            if (!indicator && owner->m_uiLayer) {
                indicator = CCLabelBMFont::create("", "chatFont.fnt");
                indicator->setID("input-buffer-status"_spr);
                indicator->setScale(0.45f);
                indicator->setAnchorPoint({0.f, 0.f});
                indicator->setPosition({8.f, 6.f});
                indicator->setOpacity(210);
                owner->m_uiLayer->addChild(indicator.data(), 100);
            }
            if (!indicator) return;
            indicator->setVisible(Mod::get()->getSettingValue<bool>("show-overlay"));
            indicator->setString(fmt::format("PIB {} | Click {}t / Release {}t | Shift+F12\n{}",
                enabled() ? "ON" : "OFF", limit(), releaseLimit(), status).c_str());
        }

        void clear() {
            pending.clear();
            shift = 0;
            tick = 0;
            physicalDown = false;
        }

        bool supported() const {
            auto* p = owner->m_player1;
            return p && !p->m_isDead && !owner->m_isPaused &&
                !owner->m_isPlatformer && !owner->m_gameState.m_isDualMode && !p->m_isDashing;
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
            GameStateSnapshot& saved;
            explicit ProbeScope(Predictor& p) : predictor(p), saved(*p.baselineWorld) {
                p.simulating = true;
                p.collision = false;
                p.uncertain = false;
                p.killedBy = nullptr;
            }
            ~ProbeScope() {
                saved.restore(predictor.owner->m_gameState);
                predictor.simulating = false;
            }
        };

        core::Outcome probe(bool down, int delay, int horizon, uintptr_t originalObstacle) {
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

            // A delayed RELEASE must begin with the actual held state and
            // keep it until `delay`. Never simulate an invented extra press.
            if (!down && !ghost->buttonDown(PlayerButton::Jump)) return {Verdict::Unknown};

            auto* danger = reinterpret_cast<GameObject*>(originalObstacle);
            bool cleared = false;
            bool previousGround = ghost->m_isOnGround;
            bool flightStarted = !down && !previousGround;
            bool completedFlight = false;
            int stableLanding = 0;
            const bool canLand = !real->m_isDart && !real->m_isShip && !real->m_isSwing;
            const bool initialGravity = ghost->m_isUpsideDown;
            auto result = [&](Verdict kind, int steps) {
                return core::Outcome{kind, steps, reinterpret_cast<uintptr_t>(killedBy), cleared};
            };

            // All candidates cover the SAME future interval. A later crash by
            // itself does not count as a successful correction.
            for (int i = 0; i < horizon; ++i) {
                if (Clock::now() >= deadline) return result(Verdict::Timeout, i);
                if (i == delay) {
                    if (down) ghost->pushButton(PlayerButton::Jump);
                    else ghost->releaseButton(PlayerButton::Jump);
                    if (real->m_isBird || (real->m_isRobot && !previousGround)) flightStarted = true;
                }
                ghost->update(step);
                ghost->updateRotation(step);
                if (uncertain) return result(Verdict::Unknown, i);
                if (collision || ghost->m_isDead) return result(Verdict::Collision, i);
                ghost->m_collisionLogTop->removeAllObjects();
                ghost->m_collisionLogBottom->removeAllObjects();
                ghost->m_collisionLogLeft->removeAllObjects();
                ghost->m_collisionLogRight->removeAllObjects();
                owner->checkCollisions(ghost.data(), step, false);
                if (uncertain) return result(Verdict::Unknown, i);
                if (collision || ghost->m_isDead) return result(Verdict::Collision, i);

                if (danger) {
                    const auto playerBounds = ghost->getObjectRect();
                    const auto hazardBounds = danger->getObjectRect();
                    if (real->m_isGoingLeft)
                        cleared = cleared || playerBounds.getMaxX() < hazardBounds.getMinX() - 2.f;
                    else
                        cleared = cleared || playerBounds.getMinX() > hazardBounds.getMaxX() + 2.f;
                }
                const bool grounded = ghost->m_isOnGround;
                if (i >= delay && canLand) {
                    if (!grounded && previousGround) flightStarted = true;
                    if ((real->m_isBall || real->m_isSpider) && ghost->m_isUpsideDown != initialGravity)
                        flightStarted = true;
                    if (grounded && flightStarted) {
                        if (!completedFlight) {
                            // End this action at its landing. Holding the ghost
                            // forever would manufacture a SECOND cube jump and
                            // reject otherwise valid early-click corrections.
                            completedFlight = true;
                            ghost->releaseButton(PlayerButton::Jump);
                        }
                        if (++stableLanding >= 4) return result(Verdict::Landed, i + 1);
                    } else if (completedFlight) {
                        // Walking off an edge is not a stable landing.
                        stableLanding = 0;
                    }
                }
                previousGround = grounded;
            }
            return result(Verdict::Clear, horizon);
        }

        int chooseDelay(bool down) {
            const int maximum = down ? limit() : releaseLimit();
            if (!enabled() || !maximum || !supported()) return 0;
            const double budget = Mod::get()->getSettingValue<double>("prediction-budget");
            if (!prepare()) return 0;
            // Allocate the ghost and snapshot once, rather than copying all
            // owning containers again for every candidate delay.
            baselineWorld = std::make_unique<GameStateSnapshot>(owner->m_gameState);
            deadline = Clock::now() + std::chrono::microseconds(static_cast<int>(budget * 1000));
            auto* p = owner->m_player1;
            // A cube jump needs enough lookahead to see its landing. A wave
            // must normally be steered again much sooner.
            const int configuredHorizon = static_cast<int>(Mod::get()->getSettingValue<int64_t>(
                p->m_isDart || p->m_isShip || p->m_isSwing ? "flight-horizon" : "ground-horizon"));
            // Even the largest candidate must actually RELEASE and be tested
            // afterwards. Surviving while holding forever is not a release save.
            const int modeFloor = p->m_isDart ? 48 : (p->m_isShip || p->m_isSwing ? 64 : 128);
            const int horizon = std::max({configuredHorizon, maximum + 12, modeFloor});
            lastDecision = core::chooseWithExtraTick(maximum, [&](int delay, uintptr_t obstacle) {
                return probe(down, delay, horizon, obstacle);
            });
            baselineWorld.reset();
            switch (lastDecision.reason) {
                case core::Reason::Corrected:
                    status = fmt::format("{} +{}t | riesgo a {}t", down ? "Click" : "Release",
                        lastDecision.delay, lastDecision.original.ticks);
                    ++corrections;
                    break;
                case core::Reason::SafeNow: status = "Input seguro: sin espera"; break;
                case core::Reason::Budget: status = "Tiempo de calculo agotado: input directo"; break;
                case core::Reason::Uncertain: status = "Zona no simulable: input directo"; break;
                default: status = fmt::format("Riesgo a {}t: sin correccion valida", lastDecision.original.ticks); break;
            }
            refreshIndicator();
            if (Mod::get()->getSettingValue<bool>("verbose-log"))
                log::info("[PIB] {} {}: {}", mode(), down ? "PRESS" : "RELEASE", status);
            return lastDecision.delay;
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
            if (!supported() || !enabled()) {
                pending.clear();
                shift = 0;
                if (!owner->m_isPaused) {
                    status = !enabled() ? "Buffer desactivado" : "Dual, platformer o dash: input directo";
                    refreshIndicator();
                }
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
                if (down) {
                    log::info("[PIB] PRESS delayed {} ticks (limit {}). Predicted trajectory improvement.",
                        delay, limit());
                } else {
                    log::info("[PIB] RELEASE: kept holding {} extra ticks (limit {} ticks). Predicted correction.",
                        delay, releaseLimit());
                }
            }
            return true;
        }

        void beforeStep(float dt) {
            // Keep time-warp and sub-tick steps in the same units as the game.
            if (dt > 0.f && dt <= 1.f) step = dt;
            if (!supported() || !enabled()) { flush(); return; }
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
            type == GameObjectType::UserCoin || type == GameObjectType::Collectible ||
            type == GameObjectType::EnterEffectObject;
    }

    bool needsWorldSimulation(GameObject* object) {
        if (solid(object->m_objectType) || inert(object->m_objectType)) return false;
        if (object->m_objectType == GameObjectType::Modifier) {
            // Speed portals are modifiers. Ordinary colour/visual triggers
            // should not mark every nearby cube trajectory as unknowable.
            const int id = object->m_objectID;
            return id == 200 || id == 201 || id == 202 || id == 203 || id == 1334;
        }
        return true;
    }

}

namespace pib {
    class SettingsMenu;
    inline SettingsMenu* menu = nullptr;

    class SettingsMenu : public Popup {
        CCLabelBMFont* m_pressValue = nullptr;
        CCLabelBMFont* m_releaseValue = nullptr;
        CCLabelBMFont* m_enabledValue = nullptr;
        CCLabelBMFont* m_visibilityValue = nullptr;
        CCLabelBMFont* m_statusValue = nullptr;

        CCLabelBMFont* label(char const* text, float x, float y, float scale = 0.4f) {
            auto* item = CCLabelBMFont::create(text, "bigFont.fnt");
            item->setScale(scale);
            item->setPosition({x, y});
            m_mainLayer->addChild(item);
            return item;
        }

        void button(char const* text, float x, float y, int tag, float scale = 0.45f) {
            auto* sprite = ButtonSprite::create(text);
            auto* item = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(SettingsMenu::onAction));
            item->setScale(scale);
            item->m_baseScale = scale;
            item->setPosition({x, y});
            item->setTag(tag);
            m_buttonMenu->addChild(item);
        }

        void refresh() {
            m_pressValue->setString(fmt::format("{} ticks", limit()).c_str());
            m_releaseValue->setString(fmt::format("{} ticks", releaseLimit()).c_str());
            m_enabledValue->setString(enabled() ? "ACTIVADO" : "DESACTIVADO");
            m_enabledValue->setColor(enabled() ? ccColor3B{110, 255, 120} : ccColor3B{255, 130, 110});
            m_visibilityValue->setString(Mod::get()->getSettingValue<bool>("show-overlay")
                ? "Indicador visible" : "Indicador oculto");
            const auto detail = active
                ? fmt::format("{} | {} correcciones\n{}", active->mode(), active->corrections, active->status)
                : "Configura los limites y entra a un nivel";
            m_statusValue->setString(detail.c_str());
            m_statusValue->limitLabelWidth(365.f, 0.36f, 0.22f);
            if (active) active->refreshIndicator();
        }

        bool setupMenu() {
            if (!Popup::init(400.f, 310.f)) return false;
            setTitle("Predictive Input Buffer");
            label("Limites de correccion. 0 = sin ayuda para esa accion.", 200, 267, 0.29f);
            m_enabledValue = label("", 200, 245, 0.43f);
            label("Al pulsar", 66, 214, 0.42f);
            m_pressValue = label("", 205, 214, 0.5f);
            button("-", 143, 214, 5, 0.5f);
            button("+", 270, 214, 6, 0.5f);
            label("Al soltar", 66, 150, 0.42f);
            m_releaseValue = label("", 205, 150, 0.5f);
            button("-", 143, 150, 7, 0.5f);
            button("+", 270, 150, 8, 0.5f);
            constexpr int values[] = {1, 2, 4, 8, 16, 32};
            for (int i = 0; i < 6; ++i) {
                const auto caption = std::to_string(values[i]);
                button(caption.c_str(), 100.f + i * 40.f, 189.f, 100 + values[i], 0.4f);
                button(caption.c_str(), 100.f + i * 40.f, 125.f, 200 + values[i], 0.4f);
            }
            button("Activar / Desactivar", 105, 91, 1, 0.42f);
            button("Mostrar / Ocultar", 295, 91, 2, 0.42f);
            m_visibilityValue = label("", 295, 72, 0.26f);
            m_statusValue = label("", 200, 50, 0.36f);
            button("Como funciona", 112, 21, 3, 0.4f);
            button("Cerrar", 300, 21, 4, 0.4f);
            refresh();
            return true;
        }

        void onAction(CCObject* sender) {
            const int action = static_cast<CCNode*>(sender)->getTag();
            if (action == 4) { onClose(sender); return; }
            if (action == 3) {
                FLAlertLayer::create("Como funciona",
                    "Al <cy>pulsar o soltar</c>, compara tu trayectoria con retrasos de 1 hasta el limite de ticks. "
                    "Solo espera si predice una mejora. Si sueltas pronto, mantiene el boton antes de soltar.\n\n"
                    "Cube, ship, ball, UFO, robot, spider, wave y swing. El indicador explica cada decision. "
                    "Prueba 1 tick adicional en correcciones; si empeora, conserva la anterior. "
                    "<cy>Shift+F12</c> abre o cierra este menu; puedes ocultar el indicador.\n\n"
                    "No garantiza sobrevivir. Dual, platformer, dash y encuentros con portales/orbes/pads "
                    "pueden pasar sin correccion. Los objetos moviles y otros mods de fisica pueden reducir la precision.\n\n"
                    "Si abriste el menu durante un nivel, cierralo y pulsa Reanudar.", "OK")->show();
                return;
            }
            if (active) active->flush();
            if (action == 1) Mod::get()->setSettingValue<bool>("enabled", !enabled());
            else if (action == 2) Mod::get()->setSettingValue<bool>("show-overlay",
                !Mod::get()->getSettingValue<bool>("show-overlay"));
            else if (action >= 100 && action < 200)
                Mod::get()->setSettingValue<int64_t>("max-delay", core::ticks(action - 100));
            else if (action >= 200)
                Mod::get()->setSettingValue<int64_t>("release-ticks", core::ticks(action - 200));
            else if (action == 5 || action == 6)
                Mod::get()->setSettingValue<int64_t>("max-delay", core::ticks(limit() + (action == 5 ? -1 : 1)));
            else if (action == 7 || action == 8)
                Mod::get()->setSettingValue<int64_t>("release-ticks", core::ticks(releaseLimit() + (action == 7 ? -1 : 1)));
            refresh();
        }

        void onClose(CCObject* sender) override {
            if (menu == this) menu = nullptr;
            Popup::onClose(sender);
        }

        ~SettingsMenu() { if (menu == this) menu = nullptr; }

    public:
        static SettingsMenu* create() {
            auto* result = new SettingsMenu;
            if (result->setupMenu()) { result->autorelease(); return result; }
            delete result;
            return nullptr;
        }
        void dismiss() { onClose(nullptr); }
    };

    void toggleMenu() {
        if (menu) { menu->dismiss(); return; }
        if (active && !active->owner->m_isPaused) {
            active->flush();
            active->owner->pauseGame(false);
        }
        menu = SettingsMenu::create();
        if (menu) menu->show();
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
        pib::active->refreshIndicator();
        return true;
    }

    void resetLevel() {
        if (m_fields->predictor) m_fields->predictor->clear();
        PlayLayer::resetLevel();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (pib::simulating() && player == pib::active->ghost.data()) {
            pib::active->collision = true;
            pib::active->killedBy = object;
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
            else if (pib::needsWorldSimulation(object) && bounds.intersectsRect(object->getObjectRect())) {
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
        static bool menuKeyConsumed = false;
        if (key == KEY_F12) {
            if (down && !repeat && getShiftKeyPressed()) {
                menuKeyConsumed = true;
                pib::toggleMenu();
                return true;
            }
            if (menuKeyConsumed) {
                if (!down) menuKeyConsumed = false;
                return true;
            }
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
    log::info("[PIB] v4.9.1 loaded. Click {} ticks, release {} ticks. Shift+F12 opens settings; the indicator can be hidden.",
        pib::limit(), pib::releaseLimit());
}
