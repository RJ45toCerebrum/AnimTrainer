So I am starting a new project that uses RayLib. This is a very basic library and can only do simple graphics but this suites my needs.
My Focus is on making a small animation toolset. Below are the functional requirements of the system:


1) Must have the ability to bring up window and render 3D objects (only a select few types of objects though. Not that many). This is mostly taken care of by raylib, but I do need to do some management.

2) I need to be able to render a Scene DAG. I need to be able to parent and / re-parent "scene objects"


Below are the current systems I will need to make (though very likely not exhaustive):

A] RenderingSystem:
Though raylib handles much of this for me, I need still to be able to traverse the Scene tree and know what to render and what does not render.


B] SceneManager:
this is the system that builds and manipulates the scene DAG. 
For now, I wont worry at all about how scenes get saved / serialized. Not a concern at the moment.

At a high level, I was thing every single object in the software, regardless of whether it renders or not, must be present in this system.
I will call this high level object:
SceneObject

ALL SceneObjects have a transform. Regardless of whether you can see it or not. So TransformComponent is barest minimum for any SceneObject.
a SceneObject alone does nothing outside of keeping track of transform information and contain an ID for uniquely identifying it.
(maybe we have a simple human readable ID that maps to an internal ID?). It does have one more property though...
It has the ability to query Components attached to the SceneObject. Think ECS architecture. That is what I am going for here.
Components ONLY HOLD DATA. We need a way of storing this component data somewhere and each system needs to be able to get access to it.

At first, I want to keep things as simple as possible. Perhaps, nothing derives from scene object. The only thing distinguishing them
is there collection of components. So the SceneObject will only have a flat list of "scene component handles." By scene component handle,
its not the scene component directly. Rather, only a handle to be able to get it. I am not exactly sure the details yet.


C] ControlWindow:
I want to keep my application as simple as possible. There are 2 windows (and only 2 windows).
There is the main rendering window where we see the actually rendered 3D scene. And there is a ControlWindow.
The ControlWindow is where I put all the other sub-windows needed for basic operations. Such as Visualizing the Scene Hierarchy 
and selecting SceneObjects. IF you have a Scene Object selected, thats where you see your component data. For starts, 
I just need a basic way to render the scene DAG. And then select objects. LATER, more stuff will go into this window to see the actually interesting things about components.
But for now, its only going to be a basic scene dag with selection capabilities.


D] Now, with the bare necessities out of the way, we move into actual animation part of the project:

Before getting AnimationComponent, we have a bunch of stuff to do before that.
First, we need to do forward and inverse kinematics of JointBones. We need to be able to parent them in a DAG structure.
Just like Maya or blender.

ControlRigSystem:
This is the system that is primarily responsible for moving the joints properly.
Maybe later even with physics joints. But I am going to not deal with that at first.
The relationship between the ControlRig and the Joints:
ControlRig HAS-A Skeleton and Skeleton HAS-A Joints. You can not have bones without a ControlRig.
This seems to make it simple in my mind.

Ignoring the Vertex to bone binding and weighting / bone movement to vertex movement, I want to get some Simple Inverse Kinematics 
going on just the Bones skeleton. I do not need to create super complex system for this. Just the necessities to get basic inverse 
kinematics going on joints.


AnimationRecorderSystem:
Again, keeping this simple at first, it only allows for recording of a singular selected SceneObject.
It can record the animations of 2 things and 2 things only. The TransformComponent. And it can record joint movement.
I still not sure on architecture of such a system yet.

AnimationPlaybackSystem:
This is where the AnimationComponent comes into play. An animation component itself does nothing.
The AnimationPlaybackSystem needs to use the AnimationComponent to be able to locate animation assets,
load them, and play/stop them when necessary. And of course, free the resources when no longer needed.


At this point, I can get to mesh skinning and maybe other things. BUT this is ALOT to start with. So I can 
revisit the details of each system later.


This is your task:
Produce a very high level architecture diagram. I am specifically talking about
the flow of data through the systems. How do systems request information, where do I store the component data?
Does each system store the component data relevant to it?

Here is a simple thought experiment.
The rendering system needs to have access the to Mesh data. So I could store that data in the RenderingSystem.
However, if I do this, I have a problem. What happens when System A needs the component data primarily used by System B.



Practical suggestion for your project:
Use GLM across the whole codebase since it integrates naturally with RayLib and covers your rendering needs
(matrix transforms, view/projection, ray casting). When you get to ControlRigSystem and the animation systems —
where you're chaining joint transforms every frame and doing quaternion interpolation up and down a skeleton —
consider pulling in RTM just for those systems.
The two can coexist fine; RTM types live in their own namespace and you'd convert at the boundary.



