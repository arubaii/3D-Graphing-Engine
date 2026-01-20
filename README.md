3D Graphing Engine

This engine is built for visualizing explicit ($f(x,y) = z$) and implicit ($f(x,y,z) - c = 0$) surfaces.  Explicit surfaces are computed standardly by domain sampling and remeshed at runtime when the user scrolls in/out. Implicit surfaces are extracted with an octree-based [marching cubes](https://en.wikipedia.org/wiki/Marching_cubes) pipeline. The math parser for user input is custom, so it isn't as robust as other libraries that support far more sophisticated and exotic parameterized functions.

Couple of Examples: 

<img width="400" height="260" src="https://github.com/user-attachments/assets/15473746-336b-47ab-abf2-7219f47575aa" />
<img width="320" height="258" src="https://github.com/user-attachments/assets/7c89b9d2-ec54-4b7d-b3ee-a43a5ed73dc6" />
<img width="300" height="218" src="https://github.com/user-attachments/assets/dcc1ed07-03e3-4e34-92fa-af12206e8947" />
<img width="360" height="307" src="https://github.com/user-attachments/assets/2705a8ca-831a-449c-afe2-5ef0ee1b529c" />
