### BTD6FreeContinues

> [!NOTE]
> This does not rely on any external software (except MinHook and the game itself), nor is it compatible with any external mods. *This project does not use a mod loader!*

This is an ad-hoc mod for BloonsTD6 which reduces the cost of game continues to zero. There is no visual change on the game over screen, but monkey money is not deducted.

This mod is a CMake project. The only dependency the mod has is on MinHook and BTD6 itself. This software may break on future updates to BTD6. In this event, the payload may fail silently, or the game may crash when trying to continue. 

To crash on failure instead of deducting the monkey money, set the PAYLOAD_FAIL_LOUDLY preset to true in CmakePresets.json.

#### Code Rundown

This project has two main parts: the bootstrapper (exe) and the payload (dll). The bootstrapper's only purpose is to inject the payload library into the running game process on a remote thread while all other threads are paused.

The payload waits for il2cpp_init to be called using a hook. It then finds + detours the InGame.GetContinueCost function. From our detour, we return zero. We also set the continues count to -1 to avoid accruing a $100 penalty for each continue.

#### Known Bugs

- The bootstrapper may time out while waiting for the payload in some instances
- The payload's Run entrypoint relies on il2cpp_init not being called yet, which is a race condition