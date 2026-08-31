#pragma once

// Names shared between a running instance and a second launch of the exe.
// Typing `visualizer` again should toggle the overlay that is already up
// rather than starting a rival copy, so the second process signals these and
// exits. Local\ scopes them to the logon session, which is what we want: two
// users on the same machine each get their own overlay.
//
// Raw literals on purpose - these are the one place a stray escape would go
// unnoticed, because both sides would agree on the same wrong name.
namespace ipc
{
inline constexpr const wchar_t *kSingletonMutex = LR"(Local\SoundWaveVisualizerSingleton)";
inline constexpr const wchar_t *kToggleEvent = LR"(Local\SoundWaveVisualizerToggle)";
inline constexpr const wchar_t *kQuitEvent = LR"(Local\SoundWaveVisualizerQuit)";
} // namespace ipc
