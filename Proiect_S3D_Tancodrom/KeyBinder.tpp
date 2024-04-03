#pragma once

#include <iostream>
#include <variant>
#include <functional>
#include <unordered_map>

/// @class KeyBinder
/// 
/// @brief A class that handles key binds for a game with the ability to bind functions at runtime. 
/// It can handle multiple function signatures at the same time. 
/// The return type of all the bound functions has to be void. 
/// Only 1 function can be bound to a key at a time. 
/// Calling a function with a signature that is not found in the template parameters will result in a compile-time error.
/// 
/// @tparam FuncSignatures The function signatures that the KeyBinder class will handle.
template<typename KeyType, typename... FuncSignatures>
class KeyBinder
{
public:
	using FuncTypesVariant = std::variant<std::function<FuncSignatures>...>;

	/// @brief Calls the function bound to the key with the given parameters.
	/// @returns true if the function was called, false if there was no function bound to the key or the signature did not match the parameters.
	template<typename... ParamTypes>
	inline bool Call(const KeyType& key, ParamTypes... params) const
	{
		if (auto it = m_keyBindsMap.find(key);
			it != m_keyBindsMap.end())
		{
			if (auto* func = std::get_if<std::function<void(ParamTypes...)>>(&it->second);
				func != nullptr)
			{
				(*func)(params...);
				return true;
			}
			else
			{
				std::cout << "The function signature void(";
				((std::cout << typeid(ParamTypes).name() << ", "), ...);
				std::cout << "\b\b) does not match with the current function signature for the key " << key;

				// TODO: print the current function signature
				// std::cout << ": " << typeid(it->second).name();

				std::cout << std::endl;
				return false;
			}
		}

		return false;
	}

	/// @brief Forces the bind of a function to a key. Overrides any existing function bound to the key.
	/// @returns void
	inline void ForceBind(const KeyType& key, FuncTypesVariant func)
	{
		m_keyBindsMap[key] = func;
	}


	/// @brief Binds a function to a key only if there is no function bound to the key.
	/// @returns true if the function was bound, false if there was already a function bound to the key.
	inline bool BindIf(const KeyType& key, FuncTypesVariant func)
	{
		if (m_keyBindsMap.find(key) == m_keyBindsMap.end())
		{
			m_keyBindsMap[key] = func;
			return true;
		}

		return false;
	}

	/// @brief Unbinds the function bound to the key.
	/// @returns true if there was a function bound, false if there was no function bound to the key.
	inline bool Unbind(const KeyType& key)
	{
		if(auto it = m_keyBindsMap.find(key);
			it != m_keyBindsMap.end())
		{
			m_keyBindsMap.erase(it);
			return true;
		}

		return false;
	}

	/// @brief Gets the function bound to the key.
	/// @returns The function bound to the key, or an empty variant if there is no function bound to the key.
	inline FuncTypesVariant Get(const KeyType& key) const
	{
		if (auto it = m_keyBindsMap.find(key);
			it != m_keyBindsMap.end())
		{
			return it->second;
		}

		return FuncTypesVariant();
	}

private:
	std::unordered_map<KeyType, FuncTypesVariant> m_keyBindsMap;
};
