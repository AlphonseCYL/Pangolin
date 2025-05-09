/* This file is part of the Pangolin Project.
 * http://github.com/stevenlovegrove/Pangolin
 *
 * Copyright (c) 2014 Steven Lovegrove
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <map>
#include <vector>
#include <memory>
#include <pangolin/platform.h>
#include <pangolin/var/varvalue.h>
#include <pangolin/utils/file_utils.h>

namespace pangolin
{

typedef void (*NewVarCallbackFn)(void* data, const std::string& name, const std::shared_ptr<VarValueGeneric>& var, bool brand_new);
typedef void (*GuiVarChangedCallbackFn)(void* data, const std::string& name, const std::shared_ptr<VarValueGeneric>& var);

struct PANGOLIN_EXPORT NewVarCallback
{
    NewVarCallback(const std::string& filter, NewVarCallbackFn fn, void* data)
        :filter(filter),fn(fn),data(data) {}
    std::string filter;
    NewVarCallbackFn fn;
    void* data;
};

struct PANGOLIN_EXPORT GuiVarChangedCallback
{
    GuiVarChangedCallback(const std::string& filter, GuiVarChangedCallbackFn fn, void* data)
        :filter(filter),fn(fn),data(data) {}
    std::string filter;
    GuiVarChangedCallbackFn fn;
    void* data;
};

class PANGOLIN_EXPORT VarState
{
public:
    static VarState& I();

    template<typename T>
    static std::string GetAttachedName(const T& backing) {
      auto* var = VarState::I()[&backing];
      if (var == nullptr) {
        return "";
      }
      return var->Meta().full_name;
    }

    template<typename T>
    static void SetAttachedValue(T& backing, T value) {
      backing = value;
      auto* var = VarState::I()[&backing];
      if (var == nullptr) {
        return;
      }
      var->Meta().gui_changed = true;
    }

    template<typename T>
    static bool AttachedChanged(T& v) {
      auto* var = VarState::I()[&v];
      if (var == nullptr) {
        return false;
      }
      bool result = var->Meta().gui_changed;
      var->Meta().gui_changed = false;
      return result;
    }


    VarState();
    ~VarState();

    void Clear();

    template<typename T>
    void NotifyNewVar(const std::string& name, std::shared_ptr<VarValue<T>>& var )
    {
        // map the var.value pointer to the name so we can lookup by backing value.
        backedNames[&var->Get()] = name;
        var_adds.push_back(name);

        // notify those watching new variables
        for(std::vector<NewVarCallback>::iterator invc = new_var_callbacks.begin(); invc != new_var_callbacks.end(); ++invc) {
            if( StartsWith(name,invc->filter) ) {
               invc->fn( invc->data, name, var, true);
            }
        }
    }

    std::shared_ptr<VarValueGeneric>& operator[](const std::string& str)
    {
        VarStoreContainer::iterator it = vars.find(str);
        if (it == vars.end()) {
            vars[str] = nullptr;
        }
        return vars[str];
    }

    // do not allow the map to be set using this operator.
    std::shared_ptr<VarValueGeneric> operator[](const void* backingValue) const
    {
        auto nameIter = backedNames.find(backingValue);
        if (nameIter == backedNames.end()) {
          return nullptr;
        }
        auto varIter = vars.find(nameIter->second);
        if (varIter == vars.end()) {
          return nullptr;
        }
        return varIter->second;
    }

    bool Exists(const std::string& str) const
    {
        return vars.find(str) != vars.end();
    }

    void FlagVarChanged()
    {
        varHasChanged = true;
    }

    bool VarHasChanged()
    {
        const bool has_changed = varHasChanged;
        varHasChanged = false;
        return has_changed;
    }

//protected:
    typedef std::map<std::string, std::shared_ptr<VarValueGeneric>> VarStoreContainer;
    typedef std::vector<std::string> VarStoreAdditions;
    typedef std::map<const void*, std::string> BackingNameMap;

    VarStoreContainer vars;
    VarStoreAdditions var_adds;
    BackingNameMap backedNames;

    std::vector<NewVarCallback> new_var_callbacks;
    std::vector<GuiVarChangedCallback> gui_var_changed_callbacks;

    bool varHasChanged;
};

inline bool GuiVarHasChanged() {
    return VarState::I().VarHasChanged();
}

inline void FlagVarChanged() {
    VarState::I().FlagVarChanged();
}

}
