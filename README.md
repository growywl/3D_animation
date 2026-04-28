## Skeletal Animation Combat Demo (Based on LearnOpenGL)

โปรเจกต์นี้พัฒนาต่อจากแนวทางตัวอย่าง skeletal animation ของ LearnOpenGL โดยทำเป็นเดโมมินิเกมต่อสู้แบบง่าย ๆ (มี player/enemy, กล้อง third-person, โจมตี, เลือด, และแถบ HP)

https://github.com/user-attachments/assets/fc22d17b-5e80-4295-8fd6-5d5f8c83c5e0
### Demo Features
- Player model: Farmer (FBX) + multiple animations
  - idle / walk / run / attack
- Enemy model: Vampire (DAE) + dancing animation
- กล้องแบบ third-person ตามผู้เล่น
  - Mouse หมุนกล้อง, Scroll ซูมเข้า/ออก
- ระบบโจมตี/โดนดาเมจ
  - ตีโดนในช่วงเวลา (damage window) ของท่าโจมตี
- Enemy HP = 3 (ตี 3 ครั้งตาย)
- แถบ HP ลอยเหนือหัว enemy (billboard health bar)
- ชนะแล้วขึ้นข้อความที่ “Run/Console panel” และเปลี่ยน title bar เป็น `YOU WIN!`
  - กด `R` เพื่อ restart/respawn

### Requirement Mapping (ตรงโจทย์)
**Load 3D model + animation from files**
- Farmer model:
  - `resources/FarmerPack/Character/Farmer/SKM_Farmer_male.fbx`
- Farmer animations:
  - `resources/FarmerPack/Animations/Farmer/anim_Farmer_idle_combat.FBX`
  - `resources/FarmerPack/Animations/Farmer/anim_Farmer_walk.FBX`
  - `resources/FarmerPack/Animations/Farmer/anim_Farmer_run.FBX`
  - `resources/FarmerPack/Animations/Farmer/anim_Farmer_attack_A.FBX`
- Vampire model + animation:
  - `resources/objects/vampire/dancing_vampire.dae`

**Camera / interaction**
- third-person camera ติดตามผู้เล่นแบบ orbit
- รองรับ zoom และ rotate ด้วย mouse

**Gameplay logic**
- โจมตี enemy ได้, enemy มี HP และ respawn ได้

### Controls
- Move: `W A S D`
- Attack: `Space`
- Respawn enemy / restart after win: `R`
- Camera rotate: `Mouse move`
- Exit: `ESC`

### Project Files
- Code:
  - `src/8.guest/2020/skeletal_animation/skeletal_animation.cpp`
- Shaders:
  - `src/8.guest/2020/skeletal_animation/anim_model.vs`
  - `src/8.guest/2020/skeletal_animation/anim_model.fs`
  - `src/8.guest/2020/skeletal_animation/hp_bar.vs`
  - `src/8.guest/2020/skeletal_animation/hp_bar.fs`
- Animation runtime / loader:
  - `includes/learnopengl/model_animation.h`
  - `includes/learnopengl/animation.h`
  - `includes/learnopengl/animator.h`
  - `includes/learnopengl/bone.h`

### Assets
- FarmerPack:
  - `resources/FarmerPack/Character/Farmer/SKM_Farmer_male.fbx`
  - `resources/FarmerPack/Character/Farmer/T_Animalstextures.png`
  - `resources/FarmerPack/Animations/Farmer/*.FBX`
- Vampire:
  - `resources/objects/vampire/dancing_vampire.dae`
  - `resources/objects/vampire/Vampire_diffuse.png`
  - `resources/objects/vampire/Vampire_specular.png`
  - `resources/objects/vampire/Vampire_normal.png`
  - `resources/objects/vampire/Vampire_emission.png`

### Credits
- Based code & tutorial: LearnOpenGL (Joey de Vries)  
  Source: `https://github.com/JoeyDeVries/LearnOpenGL`
- Special Thanks to -- Farmer Character Model
  Source: https://www.fab.com/listings/262fde27-6950-4030-b2b7-82892d406471 Author: Unreal Engine License: CC-BY 4.0
