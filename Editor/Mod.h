#pragma once

#include "ToolKit.h"
#include "StateMachine.h"
#include "App.h"

namespace ToolKit
{
	class Arrow2d;
	class LineBatch;

	namespace Editor
	{
		enum class ModId
		{
			Base,
			Select,
			Cursor,
			Move,
			Rotate,
			Scale
		};

		class BaseMod
		{
		public:
			BaseMod(ModId id);
			virtual ~BaseMod();

			virtual void Init();
			virtual void UnInit();
			virtual void Update(float deltaTime);
			virtual void Signal(SignalId signal);

		protected:
			static int GetNextSignalId();

		public:
			ModId m_id;
			StateMachine* m_stateMachine;

			// Signals.
			static SignalId m_leftMouseBtnDownSgnl;
			static SignalId m_leftMouseBtnUpSgnl;
			static SignalId m_leftMouseBtnDragSgnl;
			static SignalId m_mouseMoveSgnl;
			static SignalId m_backToStart;
		};

		class ModManager
		{
		public:
			~ModManager();

			ModManager(ModManager const&) = delete;
			void operator=(ModManager const&) = delete;

			void Init();
			void UnInit();
			static ModManager* GetInstance();
			void Update(float deltaTime);
			void DispatchSignal(SignalId signal);
			void SetMod(bool set, ModId mod); // If set true, sets the given mod. Else does nothing.

		private:
			ModManager();
			
		private:
			static ModManager m_instance;
			bool m_initiated;

		public:
			std::vector<BaseMod*> m_modStack;
		};

		// States.
		class StateType
		{
		public:
			const static std::string Null;
			const static std::string StateBeginPick;
			const static std::string StateBeginBoxPick;
			const static std::string StateEndPick;
			const static std::string StateBeginMove;
			const static std::string StateMoveTo;
			const static std::string StateEndMove;
		};

		class StatePickingBase : public State
		{
		public:
			StatePickingBase();
			virtual void TransitionIn(State* prevState) override;
			virtual void TransitionOut(State* nextState) override;
			bool IsIgnored(EntityId id);
			void PickDataToEntityId(std::vector<EntityId>& ids);

		public:
			// Picking data.
			std::vector<glm::vec2> m_mouseData;
			std::vector<Scene::PickData> m_pickData;
			std::vector<EntityId> m_ignoreList;

			// Debug models.
			static std::shared_ptr<Arrow2d> m_dbgArrow;
			static std::shared_ptr<LineBatch> m_dbgFrustum;
		};

		class StateBeginPick : public StatePickingBase
		{
		public:
			virtual void Update(float deltaTime) override;
			virtual std::string Signaled(SignalId signal) override;
			virtual std::string GetType() override { return StateType::StateBeginPick; }
		};

		class StateBeginBoxPick : public StatePickingBase
		{
		public:
			virtual void Update(float deltaTime) override;
			virtual std::string Signaled(SignalId signal) override;
			virtual std::string GetType() override { return StateType::StateBeginBoxPick; }

		private:
			void GetMouseRect(glm::vec2& min, glm::vec2& max);
		};

		class StateEndPick : public StatePickingBase
		{
		public:
			virtual void Update(float deltaTime) override;
			virtual std::string Signaled(SignalId signal) override;
			virtual std::string GetType() override { return StateType::StateEndPick; }
		};

		// Mods.
		class SelectMod : public BaseMod
		{
		public:
			SelectMod() : BaseMod(ModId::Select) { Init(); }

			virtual void Init() override;
			virtual void Update(float deltaTime) override;
		};

		class CursorMod : public BaseMod
		{
		public:
			CursorMod() : BaseMod(ModId::Cursor) { Init(); }

			virtual void Init() override;
			virtual void Update(float deltaTime) override;
		};
	}
}
