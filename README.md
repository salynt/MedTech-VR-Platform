# Unreal Engine VR 3D Medtech Brain Tumor Visualizer Client

A 3D Unreal Engine MRI brain-tumor segmentation, and visualization platform

# System Architecture:
The 3D Visualization Platform is separated into two repositories:

1. Medtech MRI 3D Visualization Backend Pipeline
2. Unreal Engine VR 3D Medtech Brain Tumor Visualization Client

# Features

- Retrieves .obj files from the **# 3D Medtech Brain Tumor Visualizer - Backend" pipeline
- Walk around and inspect reconstructed patient brain models in VR
- Visualize segmented tumors with color-coded classifications
- Toggle anatomical structures and tumor regions
- Explore axial, coronal, and sagittal slice perspectives

---

# Pipeline Flow
```
MRI / DICOM Data
        ↓
FastAPI Backend Processing
        ↓
Segmentation & Preprocessing
        ↓
Marching Cubes Mesh Extraction
        ↓
OBJ Mesh Export
        ↓
Unreal Engine VR Visualization
```

# Tech Stack


| Category | Technologies |
|---|---|
| Engine | Unreal Engine 5 |
| VR Support | OpenXR, Meta Quest / PCVR |
| Rendering | Unreal Engine Material System |
| 3D Visualization | OBJ Mesh Rendering |
| Interaction | VR locomotion and object interaction |
| Medical Visualization | Tumor segmentation visualization |
| Anatomical Navigation | Axial, Coronal, Sagittal slicing |

---

# Setup

## Requirements

- Unreal Engine 5
- VR-ready PC
- OpenXR compatible headset
- Backend visualization pipeline running locally

## Launch

1. Start the FastAPI backend pipeline
2. Open `BrainTumorPrototype.uproject`
3. Run 'Medtech MRI 3D Visualization Backend Pipeline' 
3. Launch the Unreal Engine editor
4. Start VR Preview mode
5. Load generated MRI mesh assets

# Project Structure

```txt
project_root/
│
├── Config/
├── Content/
│
├── Source/
│   ├── Private/
│   └── Public/
│
├── BrainTumorPrototype.sln
├── BrainTumorPrototype.uproject
├── README.md
└── StartBackend.bat