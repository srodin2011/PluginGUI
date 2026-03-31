#pragma once

#include "Animation.h"
#include "GdiplusExtention.h"
#include <atomic>
#include <memory>
#include <functional>
#include <map>
#include "PluginControlFactory.h"
#include "Container.h"
#include "Event.h"
#include "Frame.h"
#include "Descriptor.h"
#include "PropertyTypes.h"
#include <variant>

#pragma comment(lib, "gdiplus.lib")

// В заголовочном файле, ДО #include <gdiplus.h> или сразу после
namespace PluginGUI
{
	/// <summary>
	/// Функция позволяет выполнить сравнение двух переменных одного типа если определены операторы сравнения ("!=","==")
	/// </summary>
	/// <typeparam name="T">Тип сравниваемых переменных</typeparam>
	/// <param name="p1">Параметр 1</param>
	/// <param name="p1">Параметр 2</param>
	/// <returns></returns>
	template<typename T>
	constexpr bool IsDifferent(const T& p1, const T& p2)
	{
		if constexpr (requires { {p1 != p2} -> std::convertible_to<bool>; })
			return p1 != p2;
		else if constexpr (requires { {p1 == p2} -> std::convertible_to<bool>; })
			return !(p1 == p2);
		else
			return true;  // SFINAE-friendly: эта ветка всегда валидна
	}

	// Определение свойства с событием, отслеживающим его изменение
	// Type - тип свойства
	// Name - имя свойства
	// DefaultValue - значение по умолчанию
	#define DEFINE_PROPERTY_WITH_EVENТ(Type, Name, DefaultValue) \
		protected: \
			Type m_##Name = DefaultValue; \
		public: \
			virtual Type Get##Name() const { return m_##Name; } \
			virtual void Set##Name(const Type& val) \
			{ \
				if (m_##Name!=val)\
				{ \
					Invalidate(); \
					m_##Name = val; \
					SetDirty(); \
					TRACE("Свойство %s изменилось\n", #Name); \
					Name##Changed(val); \
					Invalidate(); \
					Id id = FindPropertyIdByName(#Name); \
					if (id != -1) FirePropertyChanged(id); \
				} \
			} \
			__declspec(property(get = Get##Name, put = Set##Name)) Type Name; \
			Event<Type> Name##Changed;

	// Определение свойства 
	// Type - тип свойства
	// Name - имя свойства
	// DefaultValue - значение по умолчанию
	#define DEFINE_PROPERTY(Type, Name, DefaultValue) \
		protected: \
			Type m_##Name = DefaultValue; \
		public: \
			virtual Type Get##Name() const { return m_##Name; } \
			virtual void Set##Name(const Type& val) \
			{ \
				if (m_##Name!=val)\
				{ \
					Invalidate(); \
					m_##Name = val; \
					SetDirty(); \
					TRACE("Свойство %s изменилось\n", #Name); \
					Invalidate(); \
					Id id = FindPropertyIdByName(#Name); \
					if (id != -1) FirePropertyChanged(id); \
				} \
			} \
			__declspec(property(get = Get##Name, put = Set##Name)) Type Name;

	#define REGISTER_PROPERTY(Type, Name, ControlType, PropId) \
		private: \
			static inline bool _registered_##ControlType##_##Name = []() \
		{ \
			PropertyTable<ControlType>::Add(static_cast<Id>(PropId), ToTypeId<Type>::value, #Name); \
			return true; \
		}();


	#define DEFINE_PROPERTY_NEW(Type, Name, DefaultValue, ControlType, PropId) \
		REGISTER_PROPERTY(Type, Name, ControlType, PropId) \
		DEFINE_PROPERTY(Type, Name, DefaultValue) 

	#define DEFINE_PROPERTY_WITH_EVENТ_NEW(Type, Name, DefaultValue, ControlType, PropId) \
		REGISTER_PROPERTY(Type, Name, ControlType, PropId) \
		DEFINE_PROPERTY_WITH_EVENТ(Type, Name, DefaultValue)

	#define IMPLEMENT_BASE_FIND_PROPERTY(Type) \
		virtual const PropertyInfo* FindProperty(Id id) const \
		{ \
			return PropertyTable<Type>::Find(id); \
		} \
		virtual Id FindPropertyIdByName(const std::string& uiName) const \
		{ \
			return PropertyTable<Type>::FindIdByName(uiName); \
		} \
		virtual const std::type_info& GetPropertyOwnerType(Id id) const \
		{ \
			if (PropertyTable<Type>::Find(id)) \
				return typeid(Type); \
			return typeid(void); \
		}

	#define IMPLEMENT_OVERRIDE_FIND_PROPERTY(Type) \
		virtual const PropertyInfo* FindProperty(Id id) const override \
		{ \
			const PropertyInfo* p = PropertyTable<Type>::Find(id); \
			if (p) return p; \
			return PropertyTable<Base>::Find(id); \
		} \
		virtual Id FindPropertyIdByName(const std::string& uiName) const \
		{ \
			Id id = PropertyTable<Type>::FindIdByName(uiName); \
			if(id != -1) return id; \
			return PropertyTable<Base>::FindIdByName(uiName); \
		} \
		virtual const std::type_info& GetPropertyOwnerType(Id id) const override \
		{ \
			if (PropertyTable<Type>::Find(id)) \
				return typeid(Type); \
			return Base::GetPropertyOwnerType(id); \
		}

	class PluginView;
	class Panel;

	using Variant = std::variant<bool, Gdiplus::Color, std::wstring, int, float, Frame>;

	/// <summary>
	/// Базовый класс для всех элементов управления 
	/// </summary>
	class Control
	{
		void Init(); // Инициализация для вызова из конструктора только!

	protected:
		using IdType = ControlId;
		/// <summary>
		/// Счетчик типов элементов управления. Нужен для присвоения правильного идентификатора новому типу элемента управления
		/// </summary>
		//static inline std::atomic<IdType> m_NextControlId{ 0 }; 

		friend class PluginControlFactory;

	public:
		enum class PropertyName
		{
			pnDefault = 0,
			pnBaseColor = pnDefault,
			pnBorder,
			pnName,
			pnFocus,
			pnValueChangeMode,
			pnSelected,
			pnLastName
		};

		Control(const CRect& border);
		virtual ~Control();

		/// <summary>
		/// Обработчик смены фокуса
		/// </summary>
		/// <param name="focused">true - приобетает фокус, false - теряет фокус</param>
		virtual void OnFocusChanged(bool focused);

		// Проверит принадлежность точки контролу
		virtual bool HitTest(int x, int y)
		{
			return Border.PtInRect(CPoint(x, y));
		}; 
		void Draw(bool drawSelected = true);
		virtual void Draw(bool hasFocus, bool drawSelected) = 0;
		virtual void Invalidate() 
		{
			m_isDirty = true;
			if (m_pContainer)
			{
				m_pContainer->Invalidate(Border);
			}
		};
		virtual void OnMouseMove(UINT nFlags, CPoint p);
		virtual BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint p);
		virtual void OnLButtonDown(UINT, CPoint p);
		virtual void OnLButtonUp(UINT, CPoint);
		virtual void OnLButtonDblClk(UINT nFlags, CPoint point);
		virtual void onMouseMoveAndLButtonDown(UINT nFlags, LONG dx, LONG dy);
		virtual BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		virtual void OnMouseLeave();
		virtual void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		virtual void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		virtual void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		virtual void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		virtual void OnContextMenu(CPoint point);
		virtual void OnUpdateCommand(CCmdUI* pCmdUI);
		virtual void OnCommand(UINT nID);

		/// <summary>
		/// Требуется ли анимация при смене фокуса элемента управления
		/// </summary>
		/// <returns></returns>
		virtual bool AnimateOnFocusChange() const
		{
			return false;
		}

		DEFINE_PROPERTY_WITH_EVENТ_NEW(Gdiplus::Color, BaseColor, Gdiplus::Color(255, 255, 0, 0), Control, PropertyName::pnBaseColor)
		DEFINE_PROPERTY_WITH_EVENТ_NEW(Frame, Border, Frame(), Control, PropertyName::pnBorder)
		DEFINE_PROPERTY_NEW(std::wstring, Name, L"", Control, PropertyName::pnName)
		DEFINE_PROPERTY_WITH_EVENТ_NEW(bool, Focus, false, Control, PropertyName::pnFocus)
		DEFINE_PROPERTY_NEW(bool, ValueChangeMode, false, Control, PropertyName::pnValueChangeMode)
		DEFINE_PROPERTY_NEW(bool, Selected, false, Control, PropertyName::pnSelected)

		/// <summary>
		/// Минимально допустимая ширина элемента управления
		/// </summary>
		virtual UINT GetMinWidth() const { return 10; }
		/// <summary>
		/// Минимально допустимая высота элемента управления
		/// </summary>
		virtual UINT GetMinHeight() const { return 10; }

		const std::wstring& GetTooltipText() const { return m_tooltipText; }
		void SetTooltipText(const std::wstring& text);
		void HideTooltip();  // ✅ Новый метод для скрытия

		virtual bool isDirty() const
		{
			return m_isDirty;
		}
		virtual void SetDirty()
		{  
			m_isDirty = true;
			if (Control* parentControl = dynamic_cast<Control*>(m_pContainer))
			{
				parentControl->SetDirty();  // Рекурсивно вверх!
			}
		}

		void GetCursorPos(LPPOINT p)
		{
			assert(m_pContainer);
			m_pContainer->GetCursorPos(p);  // всегда через контейнер
		}
		void SetCursorPos(POINT p)
		{
			assert(m_pContainer);
			m_pContainer->SetCursorPos(p);  // всегда через контейнер
		}

		// Property functions

		template<typename T>
		T GetPropertyValue(Id id) const
		{
			const Variant& v = doGetPropertyValue(id);
			if (std::holds_alternative<T>(v))
				return std::get<T>(v);
			// Здесь можно либо вернуть дефолт T{}, либо кинуть исключение
			throw std::invalid_argument("Property type mismatch");
		}

		template<typename T>
		bool SetPropertyValue(Id id, const T& value)
		{
			return doSetPropertyValue(id, Variant{ value });
		}

		virtual size_t GetPropertyCount() const
		{
			// Базовый класс считает только свои свойства
			return static_cast<size_t>(PropertyName::pnLastName);
		}
		Variant GetPropertyValue(Id id) const;

		void FirePropertyChanged(Id id)
		{
			if (m_pContainer)
			{
				m_pContainer->OnChildPropertyChanged(this, id);
			}
		}

		IMPLEMENT_BASE_FIND_PROPERTY(Control)

	protected:
		std::wstring m_tooltipText = L"";

		Control();

		/// <summary>
		/// Допустимо ли для элемента управления изменение значения с помощью мышки
		/// </summary>
		/// <returns></returns>
		virtual bool СanChangeByMouse() const
		{
			return false;
		}

		void DrawBorder2(Gdiplus::Graphics&);

		bool m_isDirty = true;
		IContainer* m_pContainer = nullptr; // Указатель на PluginView
		bool m_HasFocus = false; // Элемент управления имеет фокус
		Gdiplus::Color m_BorderColor{ 255, 255, 0, 0 }; // Цвет рамки для выделения местоположения элемента управления
		//Frame Border{ 0, 0, 0, 0 };
		//CRect m_Border{ 0, 0, 0, 0 }; // Координаты элемента управления на плагине
		std::unique_ptr<Gdiplus::Bitmap> оffScreenBitmap = nullptr; // Bitmap для двойной буферизации

		std::unique_ptr<Animation> m_FocusAnimation = nullptr; // Анимация фокуса
		float m_FocusAnimationProgress = 0.0f; // Прогресс анимации в интервале [0;1]

		void DrawBitmap(const CPaintDC& dc);

		virtual Variant doGetPropertyValue(Id id) const;
		virtual bool doSetPropertyValue(Id id, const Variant& value);

		friend class PluginView;
		friend class Panel;

		//DECLARE_DESCRIPTOR()

	public:
		Gdiplus::Bitmap* GetOffscreenBitmap() const
		{
			return оffScreenBitmap.get();
		}

	}; // PlugingControl
	
	// вспомогательные функции
	Gdiplus::Color AdjustBrightness(const Gdiplus::Color& color, float brightnessFactor);
	Gdiplus::Color AdjustColor(const Gdiplus::Color& color, float hueShift, float saturationFactor, float LightnessFactor, float alpha);
	Gdiplus::Color InterpolateColors(const Gdiplus::Color& c1, const Gdiplus::Color& c2, float t);
	//void CreateRoundedRectPath(Gdiplus::GraphicsPath& path, const Gdiplus::Rect& rect, int radius);

	// Регистрация элемента управления, который может быть создан фабрикой класс
	// pluginClassId - определить Id типа элемента управления, чтобы можно было по этому Id создавать экземпляры элемента управления с помощью фабрики классов
	// Create() - функция в классе-элементе управления, позволяющаяя создать новый экземпляра элемента управления через тип элемента управления
	// PluginClass() - конструктор по умолчанию для элемента управления
	// _registered - результат регистрации элемента управления на фабрике классов
	#define REGISTER_PLUGIN(ControlClass, BaseClass, ID) \
		public: \
			static constexpr Control::IdType pluginClassId = static_cast<Control::IdType>(ID); \
			static Control* Create() { return new ControlClass(); } \
		protected: \
			ControlClass(): BaseClass() { Init(); } \
		private: \
			static inline bool _registered = []() { \
				PluginControlFactory::Instance().Add<ID, ControlClass>(); \
				return true; \
			}();
		//static inline const Control::IdType pluginClassId = Control::m_NextControlId++; \

	#define PLUGIN_GUI_BASE_FIND_PROPERTY(Type) \
		virtual const PropertyInfo* FindProperty(Id id) const override \
		{ \
			return PropertyTable<Type>::Find(id); \
		}

} // PluginGUI