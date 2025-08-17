Here are some of the plugins I can showcase. I have more, but they are under license.


Plugins:

HumansAndTraffic – an advanced version of Epic Games’ Mass Crowd and Mass Traffic plugins.

* Added Niagara-based crowd representation (Processors/MassCrowdNiagaraProcessor, Managers/NiagaraManager).

* Human color synchronization via packed custom data in particle color. Three color variations are stored in R, G, B, and A channels.

* Improved Mass Traffic intersections logic: now multiple open periods are supported per intersection.

* Optimized Mass Traffic representation: no need for physics actors anymore. Cars calculate positions and rotations directly from wheel traces (simplified using visibility and distance checks).

* Enhanced Traffic Visualization processor: packed custom data now includes car color, right vector, and additional parameters. Fixed ISM representation issues (rotation support added).

* Introduced traffic obstacles such as barriers and speed bumps.

AdvancedSessions – a small fix for the standard sessions system.

AdvancedVoiceChat – a simple, ready-to-use voice chat solution.

InteractSystem – an easy way to interact with actors via an interface.

InventorySystemPlugin – a multiplayer inventory system based on UObjects.

* Items are represented as UObjects with instanced struct data, allowing any type of item.

 Actors containing these UObjects serve as item visualizations (pickup or usable versions).

* The usable version includes a Server_UseItem function, which triggers UseItemTagged and its multicast counterpart UseItemTaggedVisualization using gameplay tags to define logic.

PhysicInteractionPlugin – multiplayer interaction with physics actors.

* An advanced version of UPhysicsHandleComponent.

* Takes into account mass and physical materials: lighter objects cannot move heavier ones.

* Mass ratio settings are customizable via curves.

PropertyCustomizationPlugin – extends customization options:

* Adds support for struct categories.

* Adds data table customization.
    
