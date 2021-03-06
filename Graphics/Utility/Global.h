#pragma once

//简单快捷的全局访问方式 但只支持根据类型设置一个指针
template<typename T>
class GlobalVariable
{
public:

	inline static void Set(T* pVariable)
	{
		if (!m_pVariable) //全局主对象只能设置不变的一个
			m_pVariable = pVariable;
	}

	inline static T* Get()
	{
		return m_pVariable;
	}
	inline static T& Ref()
	{
		return *m_pVariable;
	}
	inline static T* Array(int index)
	{
		return m_pVariable + index;
	}

private:
	static T* m_pVariable;
};

template<class T>
inline T* GlobalVariable<T>::m_pVariable = nullptr;














