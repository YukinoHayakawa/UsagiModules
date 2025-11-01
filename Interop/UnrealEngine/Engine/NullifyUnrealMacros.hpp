/*
 * NullifyUnrealMacros.hpp
 *
 * This header defines all Unreal Engine Header Tool (UHT) macros to nothing.
 * Including this file before any Unreal headers will effectively strip all
 * Unreal-specific reflection markup from the C++ compiler's view.
 *
 * This is useful for static analysis tools, code parsers, or integration
 * with other C++ libraries that might not understand or might conflict with
 * Unreal's macro-based reflection system.
 *
 * Note: This does NOT nullify module API macros (e.g., ENGINE_API) as those
 * are required by the C++ compiler and linker for DLL import/export.
 * It also does not affect UHT-recognized template types (TSubclassOf, etc.)
 * as those are valid C++ templates.
 */

#pragma once

// Define variadic macros (...) to consume all specifiers and metadata
// passed to them.

// --- Core Type Declaration Macros ---

#ifdef UCLASS
#undef UCLASS
#endif
#define UCLASS(...)

#ifdef USTRUCT
#undef USTRUCT
#endif
#define USTRUCT(...)

#ifdef UENUM
#undef UENUM
#endif
#define UENUM(...)

#ifdef UINTERFACE
#undef UINTERFACE
#endif
#define UINTERFACE(...)

#ifdef UDELEGATE
#undef UDELEGATE
#endif
#define UDELEGATE(...)

// --- Type Member Declaration Macros ---

#ifdef UPROPERTY
#undef UPROPERTY
#endif
#define UPROPERTY(...)

#ifdef UFUNCTION
#undef UFUNCTION
#endif
#define UFUNCTION(...)

#ifdef UPARAM
#undef UPARAM
#endif
#define UPARAM(...)

// --- Code Generation & Boilerplate Macros ---

#ifdef GENERATED_BODY
#undef GENERATED_BODY
#endif
#define GENERATED_BODY(...)

#ifdef GENERATED_UCLASS_BODY
#undef GENERATED_UCLASS_BODY
#endif
#define GENERATED_UCLASS_BODY(...)

#ifdef GENERATED_USTRUCT_BODY
#undef GENERATED_USTRUCT_BODY
#endif
#define GENERATED_USTRUCT_BODY(...)

#ifdef GENERATED_UINTERFACE_BODY
#undef GENERATED_UINTERFACE_BODY
#endif
#define GENERATED_UINTERFACE_BODY(...)

#ifdef GENERATED_IINTERFACE_BODY
#undef GENERATED_IINTERFACE_BODY
#endif
#define GENERATED_IINTERFACE_BODY(...)

// --- Module Implementation Macros ---

#ifdef IMPLEMENT_PRIMARY_GAME_MODULE
#undef IMPLEMENT_PRIMARY_GAME_MODULE
#endif
#define IMPLEMENT_PRIMARY_GAME_MODULE(...)

#ifdef IMPLEMENT_MODULE
#undef IMPLEMENT_MODULE
#endif
#define IMPLEMENT_MODULE(...)
