It's plugins that i can show, i have more, but they are under license.

Plugins :
  HumansAndTraffic - advanced version of epic games mass crowd and mass traffic plugin. 
    Added nigara crowd representation (Processors\MassCrowdNiagaraProcessor, Managers/NiagaraManager). 
    Add humans color sync with packed custom data via particle color. 3 variations of colors pack in particle color at r, g, b and a channels.
    Updated mass traffic intersections logic, now there can be more than one opened period in intersection. 
    Updated Mass Traffic representation, no need to use phys actors, cars trace and calcualate positions and rotations from wheels. Traces simplify from visibility and distance.
    Updated Traffic Visualization processor, packed custom data return car color, right vector of car and some more parameters, also fixed ISM representation didnt have rotation settings.
    Add traffic obstacles, barriers and speed bumps.
  AdvancedSessins - just small fix to standart sessions.
  AdvancedVoiceChat - simple voice chat solution. Add and use.
  InteractSystem - easy way yo interact with actors via interface.
  InventorySystemPlugin - multiplayer inventory based on uobjects.
    UObjects used as item representation with instanced struct data. so item can be anything.
    Actors contaiting theese UObjects called item visualization. Can be usable of just pick up versions. 
    Usable version has Server_UseItem function, this function calls UseItemTagged and UseItemTaggedVisualization(multicast version) with gameplay tag, which can be used to define logic.
  PhysicInteractionPlugin - multiplayer interaction with phys actors.
    It's advanced version of UPhysicsHandleComponent.
    Takes mass and phys materials into account. You cant move heavier objects with lightweight objects.
    Mass ratio settings from curve.
  PropertyCustomizationPlugin - just add posibility to add struct categories, also add data table customization.
    
