#pragma once

#include <type_traits>
#include <concepts>
#include <functional>
#include <vector>
#include "UIBuildMacros.hpp"

// Include GD and cocos2d classes before this


namespace uibuilder {
	using namespace cocos2d;
	using namespace cocos2d::extension;

	template <typename T> requires (std::derived_from<T, CCObject>)
	class Build;

	template <typename T>
	struct _remove_build {
		using value = std::remove_reference_t<std::remove_pointer_t<T>>;
	};

	template <typename T>
	struct _remove_build<Build<T>> {
		using value = T;
	};

	template <typename T>
	using remove_build_t = typename _remove_build<T>::value;

	template <typename T>
	struct remove_build {
		T operator()(T a) { return a; }
	};

	template <typename T>
	class BuildCallback : public CCNode {
		std::function<void(T*)> m_callback;
	 public:
	 	static BuildCallback* create(std::function<void(T*)> cb) {
	 		auto bc = new BuildCallback;

	 		if (bc && bc->init()) {
	 			bc->autorelease();
	 			bc->m_callback = cb;
	 			return bc;
	 		}

	 		CC_SAFE_DELETE(bc);
	 		return nullptr;
	 	}

	 	void onCallback(CCObject* sender) {
	 		m_callback(static_cast<T*>(sender));
	 	}
	};

	inline std::vector<void*> buildStack;

	template <typename T> requires (std::derived_from<T, CCObject>)
	class Build {
		T* m_item;
	 public:

	 	Build<T> push() {
	 		buildStack.push_back(m_item);
	 		return *this;
	 	}

	 	static Build<T> pop() {
	 		auto ret = reinterpret_cast<T*>(buildStack.back());
	 		buildStack.pop_back();

	 		return Build<T>(ret);
	 	}

	 	static T* popRaw() {
	 		auto ret = reinterpret_cast<T*>(buildStack.back());
	 		buildStack.pop_back();

	 		return ret;
	 	}

	 	Build<T> store(T*& in) {
	 		in = m_item;
	 		return *this;
	 	}

	 	template <typename ...Args> requires requires(Args... args) {
	 		T::create(args...);
	 	}
	 	static Build<T> create(Args... args) {
	 		return Build(T::create(args...));
	 	} 

		Build(T* item) : m_item(item) {}

		T* collect() {return m_item;}

		template <typename U> requires std::derived_from<T, U>
		operator U*() { return m_item; }

		// CCArray

		setter(CCArray, addItem, addObject, CCObject*);
		setter(CCArray, insertItemAt, insertObject, CCObject*, unsigned int)
		setter(CCArray, removeObject, removeObject, CCObject*)
		setter(CCArray, removeObjectAt, removeObjectAtIndex, unsigned int)
		setter(CCArray, reverse, reverseObjects)

		template <needs_base(CCArray), typename U>
		Build<U> intoItemAt(unsigned int index) {
			return Build<U>(m_item->objectAtIndex(index));
		}

		template <needs_base(CCArray), typename U>
		Build<T> forEach(std::function<void(U*)> iter) {
			for (unsigned int i = 0; i < m_item->count(); ++i) {
				iter(static_cast<U*>(m_item->objectAtIndex(i)));
			}

			return *this;
		}

		// CCNode
		setter(CCNode, pos, setPosition, CCPoint const&)
		setter(CCNode, posX, setPositionX, float)
		setter(CCNode, posY, setPositionY, float)
		setter(CCNode, scale, setScale, float)
		setter(CCNode, scaleX, setScaleX, float)
		setter(CCNode, scaleY, setScaleY, float)
		setter(CCNode, anchorPoint, setAnchorPoint, CCPoint const&)
		setter(CCNode, zOrder, setZOrder, int)
		setter(CCNode, skewX, setSkewX, float)
		setter(CCNode, skewY, setSkewY, float)
		setter(CCNode, contentSize, setContentSize, CCSize const&)
		setter(CCNode, visible, setVisible, bool)
		setter(CCNode, rotation, setRotation, float)
		setter(CCNode, child, addChild, CCNode*)
		setter(CCNode, userData, setUserData, void*)
		setter(CCNode, userObject, setUserObject, CCObject*)

		template <needs_base(CCNode), typename U>
		Build<T> parent(U newParent) {
			remove_build<U>()(newParent)->addChild(m_item);
			return *this;
		}

		template <typename U = CCNode, needs_base(CCNode)>
		Build<U> intoParent() {
			return Build<U>(static_cast<U*>(m_item->getParent()));
		}

		template <typename U, needs_base(CCNode)>
		Build<U> intoChild(unsigned int idx) {
			return Build(static_cast<U*>(m_item->getChildren()->objectAtIndex(idx)));
		}

		template <typename U, needs_base(CCNode)>
		Build<U> intoChildByTag(int tag) {
			return Build<U>(static_cast<U*>(m_item->getChildByTag(tag)));
		}

		template <needs_base(CCNode), typename U>
		Build<remove_build_t<U>> intoNewChild(U newChild, int idx = 0, int tag = 0) {
			m_item->addChild(remove_build<U>()(newChild), idx, tag);
			return Build<remove_build_t<U>>(newChild);
		}

		template <needs_base(CCNode), typename U>
		Build<remove_build_t<U>> intoNewSibling(U newSibling, int idx = 0, int tag = 0) {
			return intoParent().intoNewChild(newSibling, idx, tag);
		}

		template <needs_base(CCNode), typename U>
		Build<remove_build_t<U>> intoNewParent(U newParent) {
			remove_build<U>()(newParent)->addChild(m_item);
			return Build<remove_build_t<U>>(newParent);
		}

		template <needs_base(CCNode)>
		Build<T> pos(float x, float y) {
			return pos(ccp(x, y));
		}

		template <needs_base(CCNode)>
		Build<T> contentSize(float w, float h) {
			return contentSize(CCSize(w, h));
		}

		template <needs_base(CCNode)>
		Build<T> width(float w) {
			return contentSize(w, m_item->getContentSize().height);
		}

		template <needs_base(CCNode)>
		Build<T> height(float h) {
			return contentSize(m_item->getContentSize().width, h);
		}

		template <needs_base(CCNode)>
		Build<T> move(float x, float y) {
			return pos(m_item->getPositionX() + x, m_item->getPositionY() + y);
		}

		template <needs_base(CCNode)>
		Build<T> move(CCPoint const& p) {
			return move(p.x, p.y);
		}

		template <needs_base(CCNode)>
		Build<T> center() {
			return pos(CCPoint(CCDirector::sharedDirector()->getWinSize() / 2.0));
		}

		template <needs_base(CCNode)>
		Build<T> anchorPoint(float x, float y) {
			return anchorPoint(ccp(x, y));
		}

		template <needs_base(CCNode), typename U>
		Build<T> iterChildren(std::function<void(U*)> iter) {
			for (unsigned int i = 0; i < m_item->getChildrenCount(); ++i) {
				iter(static_cast<U*>(m_item->getChildren()->objectAtIndex(i)));
			}

			return *this;
		}

		// Geode stuff
		#ifdef GEODE_DLL
		setter(CCNode, id, setID, std::string const&)
		setter(CCNode, layout, setLayout, Layout*)

		template <needs_base(CCNode)>
		Build<T> updateLayout() {
			m_item->updateLayout();
			return *this;
		}
		#endif

		// CCRGBAProtocol
		setter(CCRGBAProtocol, opacity, setOpacity, unsigned char)
		setter(CCRGBAProtocol, color, setColor, const ccColor3B&)
		setter(CCRGBAProtocol, cascadeOpacity, setCascadeOpacityEnabled, bool)
		setter(CCRGBAProtocol, cascadeColor, setCascadeColorEnabled, bool)

		template <needs_base(CCRGBAProtocol)>
		Build<T> color(float r, float g, float b) {
			return color(ccc3(r, g, b));
		}

		// CCLayer
		setter(CCLayer, registerTouchDispatcher, registerWithTouchDispatcher)
		setter(CCLayer, touchEnabled, setTouchEnabled, bool)
		setter(CCLayer, keyboardEnabled, setKeyboardEnabled, bool)
		setter(CCLayer, mouseEnabled, setMouseEnabled, bool)
		setter(CCLayer, touchMode, setTouchMode, ccTouchesMode)
		setter(CCLayer, touchPrio, setTouchPriority, bool)
		setter(CCLayer, keypad_enabled, setKeypadEnabled, bool)

		template <needs_base(CCLayer)>
		Build<T> initTouch() {
			registerTouchDispatcher();
			CCDirector::sharedDirector()->getTouchDispatcher()->incrementForcePrio(2);
			return touchEnabled(true).mouseEnabled(true);
		}

		template <needs_base(CCLayer)>
		Build<CCScene> intoScene() {
			auto scene = CCScene::create();
			scene->addChild(m_item);
			return Build<CCScene>(scene);
		}

		// CCScene
		template <needs_base(CCScene)>
		Build<T> pushScene() {
			CCDirector::sharedDirector()->pushScene(m_item);
			return *this;
		}

		template <needs_base(CCScene)>
		Build<T> replaceScene() {
			CCDirector::sharedDirector()->replaceScene(m_item);
			return *this;
		}

		template <needs_base(CCScene)>
		Build<CCTransitionFade> fadeIn(float fade) {
			return Build<CCTransitionFade>::create(fade, m_item);
		}


		// CCMenu
		setter(CCMenu, enabled, setEnabled, bool)

		// CCMenuItem
		setter(CCMenuItem, enabled, setEnabled, bool)
		setter(CCMenuItem, target, setTarget, CCObject*, SEL_MenuHandler)

		// CCMenuItemSpriteExtra
		setter(CCMenuItemSpriteExtra, sizeMult, setSizeMult, float)

		// CCMenuItemToggler
		template <needs_same(CCMenuItemToggler)>
		static Build<T> createToggle(CCSprite* on, CCSprite* off, std::function<void(CCMenuItemToggler*)> fn) {
			auto bc = BuildCallback<CCMenuItemToggler>::create(fn);

			return Build<CCMenuItemToggler>::create(
				on,
				off,
				bc,
				menu_selector(BuildCallback<CCMenuItemSpriteExtra>::onCallback)
			).child(bc);	
		}

		// CCSprite
		template <needs_same(CCSprite)>
		static Build<T> createSpriteName(char const* frame) {
			return Build(CCSprite::createWithSpriteFrameName(frame));
		}

		setter(CCSprite, texture, setTexture, CCTexture2D*)
		setter(CCSprite, childColor, setChildColor, CCSprite*)
		setter(CCSprite, childOpacity, setChildOpacity, unsigned char)
		setter(CCSprite, textureRect, setTextureRect, const CCRect&)
		setter(CCSprite, displayFrame, setDisplayFrame, CCSpriteFrame*)
		setter(CCSprite, dirty, setDirty, bool)
		setter(CCSprite, flipX, setFlipX, bool)
		setter(CCSprite, flipY, setFlipY, bool)

		template <needs_base(CCSprite)>
		Build<CCMenuItemSpriteExtra> intoMenuItem(CCObject* target, SEL_MenuHandler selector) {
			return Build<CCMenuItemSpriteExtra>::create(m_item, m_item, target, selector);
		}

		template <needs_base(CCSprite)>
		Build<CCMenuItemSpriteExtra> intoMenuItem(std::function<void(CCMenuItemSpriteExtra*)> fn) {
			auto bc = BuildCallback<CCMenuItemSpriteExtra>::create(fn);
			m_item->addChild(bc);

			return Build<CCMenuItemSpriteExtra>::create(
				m_item,
				m_item,
				bc,
				menu_selector(BuildCallback<CCMenuItemSpriteExtra>::onCallback)
			);
		}
		

		// CCLabelProtocol
		setter(CCLabelProtocol, string, setString, const char*)

		// CCLabelBMFont
		setter(CCLabelBMFont, alignment, setAlignment, CCTextAlignment)
		setter(CCLabelBMFont, limitLabelWidth, limitLabelWidth, float, float, float)

		// CCTextInputNode
		setter(CCTextInputNode, labelColor, setLabelNormalColor, ccColor3B const&)
		setter(CCTextInputNode, placeholderColor, setLabelPlaceholderColor, ccColor3B const&)
		setter(CCTextInputNode, placeholderScale, setLabelPlaceholderScale, float)
		setter(CCTextInputNode, maxScale, setMaxLabelScale, float)
		setter(CCTextInputNode, maxWidth, setMaxLabelWidth, float)
		setter(CCTextInputNode, allowedChars, setAllowedChars, std::string const&)
		setter(CCTextInputNode, string, setString, std::string const&)
		setter(CCTextInputNode, delegate, setDelegate, TextInputDelegate*)
		setter(CCTextInputNode, trackOnClick, onClickTrackNode, bool)

		// CCScale9Sprite
		setter(CCScale9Sprite, preferredSize, setPreferredSize, CCSize const&)
		setter(CCScale9Sprite, capInsets, setCapInsets, CCRect const&)

		// Slider
		setter(Slider, value, setValue, float)
		setter(Slider, barVisible, setBarVisibility, bool)

		// ButtonSprite
		setter(ButtonSprite, string, setString, char const*)


		/// Actions

		// CCAction
		setter(CCAction, target, setTarget, CCNode*)
		setter(CCAction, tag, setTag, int)
		setter(CCAction, speedMod, setSpeedMod, float)

		template <needs_base(CCAction), typename U>
		Build<T> runFor(U node) {
			remove_build<U>()(node)->runAction(m_item);
			return *this;
		}

		// CCFollow
		setter(CCFollow, boundarySet, setBoundarySet, bool)

		// CCFiniteTimeAction
		setter(CCFiniteTimeAction, duration, setDuration, float)

		template <needs_base(CCFiniteTimeAction)>
		Build<T> reverse() {
			m_item->reverse();
			return *this;
		}

		// CCCallFunc
		setter(CCCallFunc, target, setTargetCallback, CCObject*)

		// CCCallFuncO
		setter(CCCallFuncO, object, setObject, CCObject*)

		// CCActionInterval

		template <needs_base(CCActionInterval)>
		Build<CCRepeat> repeat(int times) {
			return Build<CCRepeat>::create(m_item, times);
		}

		template <needs_base(CCActionInterval)>
		Build<CCRepeatForever> repeatForever() {
			return Build<CCRepeatForever>::create(m_item);
		}

		template <needs_base(CCActionInterval), typename U> requires std::derived_from<remove_build_t<U>, CCActionInterval>
		Build<CCSequence> sequence(U action) {
			return Build<CCSequence>(CCSequence::create(m_item, remove_build<U>()(action), nullptr));
		}

		template <needs_base(CCActionInterval)>
		Build<CCSequence> waitAfter(float d) {
			return sequence(Build<CCDelayTime>::create(d));
		}

		template <needs_base(CCActionInterval)>
		Build<CCSequence> waitBefore(float d) {
			return Build<CCDelayTime>::create(d).sequence(m_item);
		}

		// CCEaseRateAction
		setter(CCEaseRateAction, easeRate, setRate, float)

	};


	template <typename T>
	struct remove_build<Build<T>> {
		T* operator()(Build<T> a) { return a.collect(); }
	};

}

using uibuilder::Build;

#undef setter
#undef needs_base
#undef needs_same
