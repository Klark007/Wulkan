# Important

All VKW classes should have void init and most a void del function (inheritance from abstract class VKW_Object).
Want to be able to have the VKW classes as fields in classes (i.e.) engine.
Due to not being able to initialized at the beginning of the engine constructor, these classes need an init function (could also have a non default constructor but this would mean doing allocation etc twice)
Need the del due to us wanting to delete the underlying vulkan objects using a queue.

If a class needs to store another VKW class that class is passed as a (const) pointer to the init function.
Otherwise (const) references are used