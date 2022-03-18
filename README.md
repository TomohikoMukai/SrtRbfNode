# SRT-RBF Node for Maya 2020
Implematation of "Transformation Constraints Using Approximate Spherical Regression" in Maya 2020.

Publications
- Tomohiko Mukai, Transformation Constraints Using Approximate Spherical Regression, to appear.
- Tomohiko Mukai, Example-based Interlocking Control of SRT Transformations, Visual Computing 2020 (Japanese)

## How to use
1. Load "SrtRbfNode.mll" in Plug-in Manager in Maya 2020.2
2. Select a secondary transformation node and execute the MEL command "CreateSrtRbfNode" in Script Editor. SrtRbfNode will be appeared in Node Editor if succeeded,.

![CreateSrtRbfNode](https://github.com/TomohikoMukai/SrtRbfNode/blob/image/CreateSrtRbfNode.png)

3. Connect the Matrix attribute of the primary transformation node to the Input attribute of the SrtRbfNode.

4. Specify a pair of transformations of the primary and secondary node and execute the MEL command "AddSrtRbfSample". The number of "Examples" attribute of the SrtRbfNode will be incremented if succeeded.

![AddSrtRbfSample](https://github.com/TomohikoMukai/SrtRbfNode/blob/image/AddSrtRbfExample.png)

5. Once finish adding all examples, connect the Output attribute of the SrtRbfNode to the secondary transformation node via decomposeMatrix node.

![SrtRbfNodeOutput](https://github.com/TomohikoMukai/SrtRbfNode/blob/image/SrtRbfNodeOutput.png)

## Development Environment
Windows 10 + Maya 2020（Update 2）

## Release notes
- [2022.3.18] Released initial version
- [2019.8.15] Released preliminary version

