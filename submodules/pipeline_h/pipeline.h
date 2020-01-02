#pragma once

// Copyright (C) Microsoft Corporation. All rights reserved
// 
// The MIT License (MIT)
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files(the “Software”), to deal in the Software without 
// restriction, including without limitation the rights to use, copy, modify, merge, publish, 
// distribute, sublicense, and /or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or 
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


// README: This is a working draft of a "Pipeline" capability created by Microsoft in collaboration
// with the Babylon.js team. The purpose of this software is to provide a type-safe, flexible, 
// scalable way of concatenating more-or-less independent operations into a cohesive sequence. This
// code can be considered "pre-alpha"; it is not yet fully featured nor robustly tested and should
// not yet be used as a component of shipping technology. Evaluations and feedback are welcome; 
// please contact @syntheticmagus on GitHub or Twitter with any questions/comments/suggestions you
// may have.
// 
// USAGE: At present, only relatively simple usages are supported. A trivial example follows.
// 
//     #include "pipeline.h"
// 
//     #include <vector>
// 
//     PIPELINE_TYPE(Vector, std::vector<int>);
// 
//     PIPELINE_CONTEXT(Initialize, IN_CONTRACT(), OUT_CONTRACT(Vector));
//     PIPELINE_CONTEXT(Append, IN_CONTRACT(Vector), OUT_CONTRACT(Vector));
//     PIPELINE_CONTEXT(Print, IN_CONTRACT(Vector), OUT_CONTRACT());
// 
//     void Run(Initialize& context)
//     {
//         context.SetVector({});
//     }
// 
//     void Run(Append& context)
//     {
//         auto& vector = context.ModifyVector();
//         vector.push_back(static_cast<int>(vector.size()));
//     }
// 
//     void Run(Print& context)
//     {
//         std::cout << "Vector ends with " << context.GetVector().back() << std::endl;
//     }
// 
//     int main()
//     {
//         auto pipeline = Pipeline::First<Initialize>([](auto& context) { Run(context); })
//             ->Then<Append>([](auto& context) { Run(context); })
//             ->Then<Print>([](auto& context) { Run(context); })
//             ->Then<Append>([](auto& context) { Run(context); })
//             ->Then<Print>([](auto& context) { Run(context); });
// 
//         auto data = pipeline->CreateManyMap();
//         pipeline->Run(data);
// 
//         return 0;
//     }
// 
// Please note that these patterns are far from finished, and this example is far from exhaustive
// in illustrating the capabilities of this software. Experimentation, questions, and commentary
// are always welcome.

#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>

// **************************************************************************
// ***************************** TEMPLATE UTILS *****************************
// **************************************************************************

// https://stackoverflow.com/questions/56720024/how-can-i-check-type-t-is-among-parameter-pack-ts
template<class...> struct is_one_of : std::false_type {};
template<class T1, class T2> struct is_one_of<T1, T2> : std::is_same<T1, T2> {};
template<class T1, class T2, class... Ts> struct is_one_of<T1, T2, Ts...> : std::conditional<std::is_same<T1, T2>::value, std::is_same<T1, T2>, is_one_of<T1, Ts...>>::type {};

template<typename ...Ts> struct types { using type = types<Ts...>; };

template<typename...> struct insert_type_into_types;
template<typename T, typename ...Ts> struct insert_type_into_types<T, types<Ts...>> { using type = types<T, Ts...>; };

struct SentinelT {};

// *******************************************************************
// ***************************** MANYMAP *****************************
// *******************************************************************

// Forward declare of major exported types
template<typename KeyT, typename...> struct manymap;
template<typename KeyT, typename...> struct manymap_view;

template<typename...> struct maptypes_builder;
template<typename KeyT, typename ValueT, typename ...ValueTs> struct maptypes_builder<KeyT, std::map<KeyT, ValueT>, ValueTs...> :
    types<std::map<KeyT, ValueT>, ValueTs...> {};
template<typename KeyT, typename ValueT, typename ...ValueTs> struct maptypes_builder<KeyT, ValueT, ValueTs...> :
    std::conditional<is_one_of<ValueT, ValueTs...>::value,
    maptypes_builder<KeyT, ValueTs...>,
    maptypes_builder<KeyT, ValueTs..., std::map<KeyT, ValueT>>>::type {};

template<typename KeyT, typename ...Ts> struct manymap<KeyT, types<Ts...>> : Ts... {};
template<typename KeyT, typename ...Ts> struct manymap : manymap<KeyT, typename maptypes_builder<KeyT, Ts...>::type>
{
    manymap() = default;

    template<typename ValueT>
    const std::map<KeyT, ValueT>& as() const
    {
        return static_cast<const std::map<KeyT, ValueT>&>(*this);
    }

    template<typename ValueT>
    std::map<KeyT, ValueT>& as()
    {
        return static_cast<std::map<KeyT, ValueT>&>(*this);
    }
};

template<typename KeyT, typename ValueT> struct manymap_view_base { std::map<KeyT, ValueT>* view; };
template<typename KeyT, typename ...> struct manymap_view_builder;
template<typename KeyT, typename ValueT, typename ...ValueTs> struct manymap_view_builder<KeyT, manymap_view_base<KeyT, ValueT>, ValueTs...>
    : types<manymap_view_base<KeyT, ValueT>, ValueTs...> {};
template<typename KeyT, typename ValueT, typename ...ValueTs> struct manymap_view_builder<KeyT, ValueT, ValueTs...>
    : std::conditional<is_one_of<ValueT, ValueTs...>::value,
        manymap_view_builder<KeyT, ValueTs...>,
        manymap_view_builder<KeyT, ValueTs..., manymap_view_base<KeyT, ValueT>>>::type {};

template<typename KeyT, typename ...Ts> struct manymap_view<KeyT, types<Ts...>> : protected Ts... {};
template<typename KeyT, typename ...Ts>
struct manymap_view : manymap_view<KeyT, typename manymap_view_builder<KeyT, Ts...>::type>
{
    manymap_view() = delete;

    template<typename MapT>
    manymap_view(MapT& underlyingMap)
    {
        initialize<MapT, Ts...>(underlyingMap);
    }

    template<typename ValueT>
    const std::map<KeyT, ValueT>& as() const
    {
        return *manymap_view_base<KeyT, ValueT>::view;
    }

    template<typename ValueT>
    std::map<KeyT, ValueT>& as()
    {
        return *manymap_view_base<KeyT, ValueT>::view;
    }

private:
    template<typename MapT, typename ValueT, typename ...ValueTs>
    void initialize(MapT& underlyingMap)
    {
        manymap_view_base<KeyT, ValueT>::view =
            static_cast<std::map<KeyT, ValueT>*>(&underlyingMap);
        initialize<MapT, ValueTs...>(underlyingMap);
    }

    template<typename MapT>
    void initialize(MapT& underlyingMap)
    {
        // Base case, nothing to do.
    }
};

// ********************************************************************
// ***************************** CONTRACT *****************************
// ********************************************************************

// ----------------------------------------- Begin Macro Definition -----------------------------------------
#define PIPELINE_TYPE_DEFINITION(name, key, type)                                                           \
struct name                                                                                                 \
{                                                                                                           \
    using DataType = type;                                                                                  \
    using KeyType = std::array<char, sizeof(key)>;                                                          \
    static constexpr KeyType KEY{ key };                                                                    \
                                                                                                            \
    static const char* const Key()                                                                          \
    {                                                                                                       \
        return KEY.data();                                                                                  \
    }                                                                                                       \
                                                                                                            \
    template<typename T, bool> struct Getter {};                                                            \
    template<typename T> struct Getter<T, true>                                                             \
    {                                                                                                       \
        const name::DataType& Get ## name()                                                                 \
        {                                                                                                   \
            return static_cast<T*>(this)->m_data.template Get<name>();                                      \
        }                                                                                                   \
    };                                                                                                      \
    template<typename T>                                                                                    \
    using GetterT = Getter<T, is_supported<name, typename PipelineContextTraits<T>::InContract>::value>;    \
                                                                                                            \
    template<typename T, bool> struct Setter {};                                                            \
    template<typename T> struct Setter<T, true>                                                             \
    {                                                                                                       \
        void Set ## name(const name::DataType& value)                                                       \
        {                                                                                                   \
            static_cast<T*>(this)->m_data.template Set<name>(value);                                        \
        }                                                                                                   \
    };                                                                                                      \
    template<typename T>                                                                                    \
    using SetterT = Setter<T, is_supported<name, typename PipelineContextTraits<T>::OutContract>::value>;   \
                                                                                                            \
    template<typename T, bool> struct Modifier {};                                                          \
    template<typename T> struct Modifier<T, true>                                                           \
    {                                                                                                       \
        name::DataType& Modify ## name()                                                                    \
        {                                                                                                   \
            return static_cast<T*>(this)->m_data.template Modify<name>();                                   \
        }                                                                                                   \
    };                                                                                                      \
    template<typename T>                                                                                    \
    using ModifierT = Modifier<T,                                                                           \
        is_supported<name, typename PipelineContextTraits<T>::InContract>::value &&                         \
        is_supported<name, typename PipelineContextTraits<T>::OutContract>::value>;                         \
                                                                                                            \
    template<typename T>                                                                                    \
    struct AccessorT : GetterT<T>, SetterT<T>, ModifierT<T> {};                                             \
}
// ------------------------------------------ End Macro Definition ------------------------------------------

// ------------------ Begin Macro Definition --------------------
#define PIPELINE_TYPE(name, type)                               \
PIPELINE_TYPE_DEFINITION(name, #name, type)
// -------------------- End Macro Definition --------------------

// Significant types (using/forward declare).
template<typename...> struct Contract;
using InvalidContract = Contract<>;
template<typename...> struct is_supported;
template<typename...> struct compatibility;
template<typename...> struct insertion;
template<typename...> struct combination;
using KeyT = std::string;
template<typename...> struct manymap_view_from_contract;
template<typename...> struct ContractCompatibilityAnalyzer;

// Implementations below here

template<typename ...Ts> struct Contract
{
    using ManyMapViewT = typename manymap_view_from_contract<Contract<Ts...>>::type;

    template<typename T>
    using And = Contract<Ts..., T>;
};

template<size_t LeftL, size_t RightL>
constexpr bool sameCharArray(const std::array<char, LeftL>& left, const std::array<char, RightL> right)
{
    if (LeftL != RightL)
    {
        return false;
    }

    for (size_t idx = 0; idx < LeftL; ++idx)
    {
        if (left[idx] != right[idx])
        {
            return false;
        }
    }

    return true;
}

template<typename LeftT, typename RightT>
constexpr bool sameData()
{
    if (std::is_same<LeftT, RightT>::value)
    {
        return true;
    }

    if (!std::is_same<typename LeftT::DataType, typename RightT::DataType>::value)
    {
        return false;
    }

    return sameCharArray(LeftT::KEY, RightT::KEY);
}

template<typename...> struct is_supported : std::false_type {};
template<typename T1, typename T2, typename ...Ts> struct is_supported<T1, T2, Ts...> :
    std::conditional<sameData<T1, T2>(), std::true_type, is_supported<T1, Ts...>>::type {};
template<typename T, typename ...Ts> struct is_supported<T, Contract<Ts...>> : is_supported<T, Ts...> {};

template<typename ...Ts> struct compatibility<SentinelT, Ts...> : std::true_type {};
template<typename T1, typename T2, typename ...Ts> struct compatibility<T1, T2, Ts...> :
    std::conditional<compatibility<T1, Ts...>::value, compatibility<T2, Ts...>, std::false_type>::type {};
template<typename T, typename ...Ts> struct compatibility<T, SentinelT, Ts...> : is_supported<T, Ts...> {};
template<typename ...LeftTs, typename ...RightTs> struct compatibility<Contract<LeftTs...>, Contract<RightTs...>> :
    compatibility<LeftTs..., SentinelT, RightTs...> {};

template<typename T, typename ...Ts> struct insertion<Contract<Ts...>, T> { using type = Contract<Ts..., T>; };

template<typename ContractT> struct combination<ContractT> { using type = ContractT; };
template<typename ContractT, typename T, typename ...Ts> struct combination<ContractT, T, Ts...> :
    std::conditional<is_supported<T, ContractT>::value, combination<ContractT, Ts...>, combination<typename insertion<ContractT, T>::type, Ts...>>::type {};
template<typename ContractT, typename ...Ts> struct combination<ContractT, Contract<Ts...>> : combination<ContractT, Ts...> {};

template<typename ...Ts> struct manymap_view_from_contract<SentinelT, Ts...> { using type = manymap_view<KeyT, Ts...>; };
template<typename T, typename ...Ts> struct manymap_view_from_contract<T, Ts...>
    : manymap_view_from_contract<Ts..., typename T::DataType> {};
template<typename ...Ts> struct manymap_view_from_contract<Contract<Ts...>> : manymap_view_from_contract<Ts..., SentinelT> {};

template<typename> struct manymap_from_contract;
template<typename ...Ts> struct manymap_from_contract<manymap_view<Ts...>> : manymap<Ts...> {};
template<typename ...Ts> struct manymap_from_contract<Contract<Ts...>> :
    manymap_from_contract<typename manymap_view_from_contract<Contract<Ts...>>::type> {};

template<typename ContractT, typename T, typename ...Ts> struct ContractCompatibilityAnalyzer<Contract<T, Ts...>, ContractT>
{
    static constexpr void Analyze()
    {
        if (!compatibility<Contract<T>, ContractT>::value)
        {
            std::cout << "  - " << T::Key() << " is required, but is not supported by the ancestor's contract." << std::endl;
        }

        ContractCompatibilityAnalyzer<Contract<Ts...>, ContractT>::Analyze();
    }
};
template<typename ContractT> struct ContractCompatibilityAnalyzer<Contract<>, ContractT> { static constexpr void Analyze() {} };

// **************************************************************************
// ***************************** PIPELINE STATE *****************************
// **************************************************************************

// ---------------- Begin Macro Definition ------------------
#define PIPELINE_STATE_NAME(name)                           \
static constexpr std::array<char, sizeof(#name)> NAME{ #name }
// ------------------ End Macro Definition ------------------

template<typename ...Ts>
struct PipelineState
{
    PIPELINE_STATE_NAME(Malformed Pipeline State);

    using PipelineStateT = PipelineState<Ts...>;
    template<typename NextOperationT>
    using ThenT = PipelineState<NextOperationT, PipelineStateT>;

    static constexpr bool IS_COMPATIBLE{ false };
    using Contract = InvalidContract;

    static constexpr void Analyze()
    {
        static_assert(IS_COMPATIBLE, "\n\n"
            "    MALFORMED PIPELINE STATE \n"
            "    If you're seeing this message, you've probably created a pipeline state that attempts an operation \n"
            "    which doesn't conform to the expected structure. Such a state will induce the compiler to indulge \n"
            "    its passion for unintelligible template spew, leading to a gigantic litany of near-useless error \n"
            "    error messages. [INSERT RECOMMENDED COURSE OF ACTION HERE]"
            "\n\n");
    }

    static constexpr const char* OperationName()
    {
        throw new std::exception("Operation name of empty pipeline should never be requested.");
        return nullptr;
    }
};

template<>
struct PipelineState<>
{
    PIPELINE_STATE_NAME(Pipeline Start);

    using PipelineStateT = PipelineState<>;
    template<typename NextOperationT>
    using ThenT = PipelineState<NextOperationT, PipelineStateT>;

    static constexpr bool IS_COMPATIBLE{ true };
    using Contract = Contract<>;

    static /*constexpr*/ void Analyze()
    {
        std::cout << "Pipeline is empty; nothing to be incompatible with." << std::endl;
        std::cout << std::endl;
    }

    static /*constexpr*/ const char* OperationName()
    {
        throw new std::exception("Operation name of empty pipeline should never be requested.");
    }

    template<typename NextOperationT, typename CallableT>
    static std::shared_ptr<ThenT<NextOperationT>> First(CallableT&& callable)
    {
        auto shared = std::make_shared<PipelineState<>>();
        return ThenT<NextOperationT>::Create(shared, callable);
    }

    template<typename T>
    void Run(T&)
    {
        // Base case, nothing to do.
    }
};

template<typename OperationT> struct PipelineState<OperationT> : PipelineState<OperationT, PipelineState<>> {};

template<typename OperationT, typename ...PriorOperationsT>
struct PipelineState<OperationT, PipelineState<PriorOperationsT...>>
{
    using HeritageT = PipelineState<PriorOperationsT...>;
    using PipelineStateT = PipelineState<OperationT, HeritageT>;
    template<typename NextOperationT>
    using ThenT = PipelineState<NextOperationT, PipelineStateT>;

    static constexpr bool IS_COMPATIBLE{ HeritageT::IS_COMPATIBLE && compatibility<typename OperationT::InContract, typename HeritageT::Contract>::value };
    using Contract = typename std::conditional<IS_COMPATIBLE,
        typename combination<typename HeritageT::Contract, typename OperationT::OutContract>::type,
        InvalidContract>::type;

    template<typename CallableT>
    PipelineState(std::shared_ptr<HeritageT>& ancestor, CallableT&& callable)
        : m_ancestor{ ancestor }
        , m_action{ callable }
    {}

    template<typename CallableT>
    static std::shared_ptr<PipelineStateT> Create(std::shared_ptr<HeritageT>& ancestor, CallableT&& callable)
    {
        auto ptr = std::make_shared<PipelineStateT>(ancestor, callable);
        ptr->m_self = ptr;
        return ptr;
    }

    template<typename NextOperationT, typename CallableT>
    std::shared_ptr<ThenT<NextOperationT>> Then(CallableT&& callable)
    {
        auto ptr = m_self.lock();
        return ThenT<NextOperationT>::Create(ptr, callable);
    }

    // TODO: This method takes a dependency on the fact that, at present, nothing can ever 
    // be REMOVED from the contract. Consequently, the contract at the end of a pipeline 
    // can be considered to be a full representation of the types that will be needed anywhere 
    // in the pipeline. This assumption may not hold true for long; and when it no longer
    // applies, the manymap_from_contract will have to be retired in favor of a different
    // system that tracks and collects, without deletion, the types that will be required
    // throughout the pipeline.
    manymap_from_contract<Contract> CreateManyMap()
    {
        return{};
    }

    // TODO: When a task system is available, switch this to just using a task.
    template<typename DataT>
    void Run(DataT& data)
    {
        static_assert(IS_COMPATIBLE, "Contracts not compatible.");

        // TODO: This is one of the places where changes for multiple ancestry might occur.
        m_ancestor->Run(data);

        OperationT operation{ data };
        m_action(operation);
    }

    static constexpr const char* OperationName()
    {
        return OperationT::NAME.data();
    }

    static constexpr void Analyze()
    {
        if (PipelineStateT::IS_COMPATIBLE)
        {
            std::cout << "All pipeline operations compatible." << std::endl;
            std::cout << std::endl;
        }
        else if (HeritageT::IS_COMPATIBLE)
        {
            std::cout << "Pipeline operation " << OperationName() << " is not compatible with contract from ancestor " << HeritageT::OperationName() << std::endl;
            ContractCompatibilityAnalyzer<typename OperationT::InContract, typename HeritageT::Contract>::Analyze();
            std::cout << std::endl;
        }
        else
        {
            HeritageT::Analyze();
        }
    }

private:
    std::shared_ptr<HeritageT> m_ancestor{};
    std::function<void(OperationT&)> m_action{};
    std::weak_ptr<PipelineStateT> m_self{};
};

// ************************************************************************
// ***************************** ACCESSORIZER *****************************
// ************************************************************************

template<typename T>
struct Getter
{
    using DataT = typename T::DataType;

    template<typename MapT>
    const DataT& Get(const MapT& map) const
    {
        return map.template as<DataT>().at(T::Key());
    }
};

struct EmptyGetter {};
template<typename T, typename ViewT>
using GetterT = typename std::conditional<is_supported<T, typename ViewT::InContract>::value,
    Getter<T>,
    EmptyGetter>::type;

template<typename T>
struct Setter
{
    using DataT = typename T::DataType;

    template<typename MapT>
    void Set(const DataT& data, MapT& bag)
    {
        bag.template as<DataT>()[T::Key()] = data;
    }
};

struct EmptySetter {};
template<typename T, typename ViewT>
using SetterT = typename std::conditional<is_supported<T, typename ViewT::OutContract>::value,
    Setter<T>,
    EmptySetter>::type;

template<typename T>
struct Modifier
{
    using DataT = typename T::DataType;

    template<typename MapT>
    DataT& Modify(MapT& map)
    {
        return map.template as<DataT>().at(T::Key());
    }
};

struct EmptyModifier {};
template<typename T, typename ViewT>
using ModifierT = typename std::conditional<is_supported<T, typename ViewT::InContract>::value && is_supported<T, typename ViewT::OutContract>::value,
    Modifier<T>,
    EmptyModifier>::type;

template<typename T, typename ViewT>
struct AccessorT : GetterT<T, ViewT>, SetterT<T, ViewT>, ModifierT<T, ViewT> {};

template<typename...> struct accessors_builder;
template<typename ViewT, typename T, typename ...Ts> struct accessors_builder<ViewT, AccessorT<T, ViewT>, Ts...> :
    types<AccessorT<T, ViewT>, Ts...> {};
template<typename ViewT, typename T, typename ...Ts> struct accessors_builder<ViewT, T, Ts...> :
    accessors_builder<ViewT, Ts..., AccessorT<T, ViewT>> {};
template<typename ViewT, typename ...Ts> struct accessors_builder<ViewT, Contract<Ts...>> :
    accessors_builder<ViewT, Ts...> {};
template<typename ViewT> struct Accessors :
    accessors_builder<ViewT, typename combination<typename ViewT::InContract, typename ViewT::OutContract>::type> {};

template<typename...> struct Accessorizer;
template<typename ...Ts> struct Accessorizer<types<Ts...>> : Ts... {};
template<typename ViewT, typename MapViewT> struct Accessorizer<ViewT, MapViewT> : Accessorizer<typename Accessors<ViewT>::type>
{
    template<typename UnderlyingMapT>
    Accessorizer(UnderlyingMapT& m_map)
        : m_map{ m_map }
    {}

    template<typename T>
    const typename T::DataType& Get() const
    {
        return GetterT<T, ViewT>::Get(m_map);
    }

    template<typename T>
    void Set(const typename T::DataType& value)
    {
        return SetterT<T, ViewT>::Set(value, m_map);
    }

    template<typename T>
    typename T::DataType& Modify()
    {
        return ModifierT<T, ViewT>::Modify(m_map);
    }

private:
    MapViewT m_map;
};

// ****************************************************************************
// ***************************** PIPELINE CONTEXT *****************************
// ****************************************************************************

// ----------------------------- Begin Macro Definition -----------------------------
#define PIPELINE_CONTEXT(name, inContract, outContract)                             \
struct name;                                                                        \
template<> struct PipelineContextTraits<name>                                       \
{                                                                                   \
    using InContract = inContract;                                                  \
    using OutContract = outContract;                                                \
    using CombinedContract = typename combination<inContract, outContract>::type;   \
};                                                                                  \
struct name : PipelineContext<name>                                                 \
{                                                                                   \
    PIPELINE_STATE_NAME(name);                                                      \
    using InContract = PipelineContextTraits<name>::InContract;                     \
    using OutContract = PipelineContextTraits<name>::OutContract;                   \
    using MapViewT = PipelineContext<name>::MapViewT;                               \
}
// ------------------------------ End Macro Definition ------------------------------

#define IN_CONTRACT(...) Contract<__VA_ARGS__>
#define OUT_CONTRACT(...) Contract<__VA_ARGS__>

template<typename ...Ts> struct context_accessorizer_base_case : Ts... {};

template<typename...> struct context_accessorizer;
template<typename BaseT, typename ...Ts> struct context_accessorizer<BaseT, types<Ts...>> { using type = context_accessorizer_base_case<Ts...>; };
template<typename BaseT, typename TypesT, typename T, typename ...Ts> struct context_accessorizer<BaseT, TypesT, T, Ts...> :
    context_accessorizer<BaseT, typename insert_type_into_types<typename T::template AccessorT<BaseT>, TypesT>::type, Ts...> {};
template<typename BaseT, typename ...Ts> struct context_accessorizer<BaseT, Contract<Ts...>> : context_accessorizer<BaseT, types<>, Ts...> {};

template<typename T>
struct PipelineContextTraits;

template<typename T>
struct PipelineContext : context_accessorizer<T, typename PipelineContextTraits<T>::CombinedContract>::type
{
    using InContract = typename PipelineContextTraits<T>::InContract;
    using OutContract = typename PipelineContextTraits<T>::OutContract;
    using MapViewT = typename combination<InContract, OutContract>::type::ManyMapViewT;

    Accessorizer<PipelineContextTraits<T>, MapViewT> m_data;

    template<typename DataT>
    PipelineContext(DataT& map)
        : m_data{ map }
    {}
};
// ********************************************************************
// ***************************** PIPELINE *****************************
// ********************************************************************

using Pipeline = PipelineState<>;