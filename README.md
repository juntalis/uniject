# Uniject

**NOTE**: Nearly complete, but still a WIP

Windows-specific command line tool and library for injecting assemblies into Unity games at runtime. (possibly any
Mono application, but haven't tested) Useful when you want to load an assembly into a game but don't want to patch
the game's files.

## Usage

Usage information specific to the library or command line tool found further down. This section describes the options and expectations.

**Required**:

The following parameters require a value:

* `pid` - The targeted process ID.
* `assembly_name` - The assembly you'd like to load. (not required, but highly recommended that you use the absolute filepath)

You can use whatever assembly/class/method name you'd like, but the signature of the entrypoint method for your assembly
is expected to match:

```csharp
public static void MethodName()
{
    // ...
}
```

**Optional**:

The following parameters are optional:

* `mono_name` (Default: mono.dll) - The Mono DLL filename. The default should cover _most_ games, but I've seen other names used.
* `class_name` (Default: Loader) - The fully qualified name of the class that contains your assembly's entrypoint method.
* `method_name` (Default: Initialize) - The name of your entrypoint method, which is immediately called after the assembly has been loaded.
* `thread_id` (No Default) - Should only be specified if you require your assembly be loaded by a specific thread in the target process.
* `debugging` (Default: FALSE) - Allows debugging the game in Visual Studio Tools for Unity. See below for more info.
* `log_path` (Default: Auto-generated) - Allows overriding the filepath of the logfile that doesn't currently exist.

**Unsupported**:

The following parameters were either started and scrapped or never made it past "idea". 

* `no_wait` (Default: FALSE) - Tells the injector not to wait for confirmation that the the loader has completed. 

**Debugging**

I copied the logic for this option from [dynity](https://github.com/HearthSim/dynity.git), which documents the setup process. I've never tried setting up
the tools and testing this.

## Command Line Usage

TODO: Command line documentation

## Library Usage

TODO: Library documentation


## How it Works

Probably best to just break this down into the step-by-step:

* Injector is fed and processes a set of parameters for the current injection.
* Injector initializes shared memory and populates it with necessary information for the current injection.
* Injector creates a named event that will be used to signal the completion of the loader process.
* Injector acquires necessary token privileges and opens the process specified by the PID parameter.
* Injection of the loader DLLs:
  * `uniject-loader32.dll` vs `uniject-loader64.dll` - Picked based on whether the process is 32-bit or 64-bit.
  * `SetThreadContext` vs `CreateRemoteThread` - Thread context based injection is only used if the parameters specify a thread to load in.
* Loader (DLL) maps the shared memory and reads in the injection instructions.
* Loader finds the currently loaded Mono DLL and looks up all the API it'll be using.
* If the loader was injected using `CreateRemoteThread`, it attaches the thread to the root app domain.
* Loader finds the specified assembly and loads it into the current app domain.
* Loader searches the newly loaded assembly for the specified class and method name, then makes a call and wait for it to complete.
* If the loader was injected using `CreateRemoteThread`, it detaches the thread from the root app domain.
* Loader cleans up, signals the named event to notify the injector of its completion, and then unloads itself from the process. (assembly still loaded)
* Injector cleans up shared memory and event and exits. 
