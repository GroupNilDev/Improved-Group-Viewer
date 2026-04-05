#include <Geode/Geode.hpp>
#include <Geode/modify/SetGroupIDLayer.hpp>
#include <Geode/modify/CCScrollLayerExt.hpp>

using namespace geode::prelude;

class $modify(ImprovedSetGroupIDLayer, SetGroupIDLayer) {
    struct Fields {
        Ref<ScrollLayer> m_scrollLayer;
        Ref<CCMenu> m_scrollableMenu;
        float m_lastScrollPos = 0;
    };

    bool init(GameObject* obj, cocos2d::CCArray* objs) {
        if (!SetGroupIDLayer::init(obj, objs)) {
            return false;
        }

        // Find the original groups menu
        CCMenu* originalMenu = nullptr;
        for (CCNode* child : CCArrayExt<CCNode*>(m_mainLayer->getChildren())) {
            if (child->getID() == "groups-list-menu") {
                originalMenu = typeinfo_cast<CCMenu*>(child);
                break;
            }
        }

        if (!originalMenu) return true;

        // Save original position and hide it
        CCPoint originalPos = originalMenu->getPosition();
        originalMenu->setVisible(false);

        // Count how many buttons
        int buttonCount = originalMenu->getChildrenCount();
        
        // Calculate scroll height based on button count
        float scrollHeight = 68.f; // Default
        if (buttonCount > 10) scrollHeight = 110.f;
        if (buttonCount > 20) scrollHeight = 152.f;
        if (buttonCount > 30) scrollHeight = 194.f;

        // Create scroll layer
        auto scrollLayer = ScrollLayer::create({360.f, scrollHeight});
        scrollLayer->setPosition({CCDirector::get()->getWinSize().width / 2, 
                                   CCDirector::get()->getWinSize().height / 2 - 16.8f});
        scrollLayer->ignoreAnchorPointForPosition(false);
        scrollLayer->setID("groups-scroll-layer"_spr);

        // Create container
        auto container = CCNode::create();

        // Create new menu
        auto newMenu = CCMenu::create();
        newMenu->setPosition(originalPos);
        
        // Copy all buttons from original menu
        for (CCNode* child : CCArrayExt<CCNode*>(originalMenu->getChildren())) {
            auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
            if (button) {
                button->retain();
                button->removeFromParent();
                newMenu->addChild(button);
                button->release();
            }
        }

        // Setup layout
        auto layout = RowLayout::create();
        layout->setGap(12.f);
        layout->setAutoScale(false);
        layout->setGrowCrossAxis(true);
        layout->setCrossAxisOverflow(true);
        layout->setAxisAlignment(AxisAlignment::Start);
        newMenu->setLayout(layout);
        newMenu->updateLayout();

        // Calculate container height
        float containerHeight = newMenu->getScaledContentSize().height + 10.f;
        container->setContentSize({360.f, containerHeight});
        container->addChild(newMenu);
        
        newMenu->setPosition({container->getContentSize().width / 2, containerHeight - 5.f});
        newMenu->setAnchorPoint({0.5f, 1.f});

        // Add to scroll layer
        scrollLayer->m_contentLayer->addChild(container);
        scrollLayer->m_contentLayer->setContentSize(container->getContentSize());
        
        // Restore scroll position if saved
        if (m_fields->m_lastScrollPos != 0) {
            scrollLayer->m_contentLayer->setPositionY(m_fields->m_lastScrollPos);
        }

        m_mainLayer->addChild(scrollLayer);

        // Store references
        m_fields->m_scrollLayer = scrollLayer;
        m_fields->m_scrollableMenu = newMenu;

        // Adjust background size if needed
        if (CCNode* bg = m_mainLayer->getChildByID("groups-bg")) {
            if (scrollHeight > 68.f) {
                auto bgSize = bg->getContentSize();
                bg->setContentSize({bgSize.width, scrollHeight + 20.f});
                bg->setPositionY(bg->getPositionY() - 10.f);
            }
        }

        // Update touch priority so scrolling works
        handleTouchPriority(this);

        #ifndef GEODE_IS_IOS
        queueInMainThread([this, scrollLayer] {
            if (auto delegate = typeinfo_cast<CCTouchDelegate*>(scrollLayer)) {
                if (auto handler = CCTouchDispatcher::get()->findHandler(delegate)) {
                    CCTouchDispatcher::get()->setPriority(handler->getPriority() - 1, handler->getDelegate());
                }
            }
        });
        #endif

        return true;
    }

    // Save scroll position when layer is destroyed
    void onExit() override {
        if (m_fields->m_scrollLayer) {
            m_fields->m_lastScrollPos = m_fields->m_scrollLayer->m_contentLayer->getPositionY();
        }
        SetGroupIDLayer::onExit();
    }
};

// Handle button disabling while scrolling
class $modify(MyCCScrollLayerExt, CCScrollLayerExt) {
    void ccTouchMoved(CCTouch* touch, CCEvent* event) {
        CCScrollLayerExt::ccTouchMoved(touch, event);
        
        // Find the SetGroupIDLayer and disable its buttons
        auto scene = CCDirector::get()->getRunningScene();
        if (auto layer = scene->getChildByType<SetGroupIDLayer>(0)) {
            if (auto menu = layer->getChildByID("groups-scroll-layer"_spr)) {
                // Disable buttons while scrolling
                auto scrollLayer = typeinfo_cast<ScrollLayer*>(menu);
                if (scrollLayer && scrollLayer->m_contentLayer) {
                    for (auto child : CCArrayExt<CCNode*>(scrollLayer->m_contentLayer->getChildren())) {
                        if (auto container = typeinfo_cast<CCNode*>(child)) {
                            for (auto menuChild : CCArrayExt<CCMenuItemSpriteExtra*>(container->getChildren())) {
                                menuChild->setEnabled(false);
                            }
                        }
                    }
                }
            }
        }
    }
    
    void ccTouchEnded(CCTouch* touch, CCEvent* event) {
        CCScrollLayerExt::ccTouchEnded(touch, event);
        
        // Re-enable buttons
        auto scene = CCDirector::get()->getRunningScene();
        if (auto layer = scene->getChildByType<SetGroupIDLayer>(0)) {
            if (auto menu = layer->getChildByID("groups-scroll-layer"_spr)) {
                auto scrollLayer = typeinfo_cast<ScrollLayer*>(menu);
                if (scrollLayer && scrollLayer->m_contentLayer) {
                    for (auto child : CCArrayExt<CCNode*>(scrollLayer->m_contentLayer->getChildren())) {
                        if (auto container = typeinfo_cast<CCNode*>(child)) {
                            for (auto menuChild : CCArrayExt<CCMenuItemSpriteExtra*>(container->getChildren())) {
                                menuChild->setEnabled(true);
                            }
                        }
                    }
                }
            }
        }
    }
};
