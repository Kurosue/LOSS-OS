# Final Project - IF2130 Operating System: Loss
> Yes, we made our own OS from stratch _(not really tho, but still)_.

<br>
<p align="center">
    <img width="150px" src="https://github.com/user-attachments/assets/1b70374f-702d-4bfe-8011-72c91f92a8fb">
</p>

<div align="center">
  <h3 align="center"> Tech Stacks </h3>

  <p align="center">
    
[![AssemblyScript](https://img.shields.io/badge/assembly%20script-%23000000.svg?style=for-the-badge&logo=assemblyscript&logoColor=white)][Asm-url]
[![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)][C-url]
  
  </p>
</div>

---

### Table of Contents <a name="table-of-contents"></a>

- [Contributors](#contributors)
- [About](#about)
- [Prologue](#prologue)
- [Milestones](#milestones)
- [The OS](#os)
- [How to Run?](#run)
- [Acknowledgements](#acknowledgements)
---

### Contributors <a name="contributors"></a>

<!-- CONTRIBUTOR -->
 <div align="center" id="contributor">
   <strong>
     <h3> Team 10: Loss </h3>
     <table align="center">
       <tr align="center">
         <td>NIM</td>
         <td>Name</td>
         <td>GitHub</td>
       </tr>
       <tr align="center">
         <td>13523002</td>
         <td>Refki Alfarizi</td>
         <td align="center" >
           <div style="margin-right: 20px;">
           <a href="https://github.com/l0stplains" ><img src="https://avatars.githubusercontent.com/u/78079998?v=4" width="48px;" alt=""/> 
             <br/> <sub><b> @l0stplains </b></sub></a><br/>
           </div>
         </td>
       </tr>
       <tr align="center">
         <td>13523004</td>
         <td>Razi Rachman Widyadhana</td>
         <td align="center" >
           <div style="margin-right: 20px;">
           <a href="https://github.com/zirachw" ><img src="https://avatars.githubusercontent.com/u/148220821?v=4" width="48px;" alt=""/> 
             <br/> <sub><b> @zirachw </b></sub></a><br/>
           </div>
         </td>
       </tr>
       <tr align="center">
         <td>13523028</td>
         <td>Muhammad Aditya Rahmadeni</td>
         <td align="center" >
           <div style="margin-right: 20px;">
           <a href="https://github.com/Kurosue" ><img src="https://avatars.githubusercontent.com/u/68522463?v=4" width="48px;" alt=""/> 
             <br/> <sub><b> @Kurosue </b></sub></a><br/>
           </div>
         </td>
       </tr>
       <tr align="center">
         <td>13523044</td>
         <td>Muhammad Luqman Hakim</td>
         <td align="center" >
           <div style="margin-right: 20px;">
           <a href="https://github.com/mlqmn" ><img src="https://avatars.githubusercontent.com/u/163738027?v=4" width="48px;" alt=""/> 
             <br/> <sub><b> @mlqmn </b></sub></a><br/>
           </div>
         </td>
       </tr>
     </table>
   </strong>
 </div>

<br>

---

<!-- ABOUT -->
### About <a name="about"></a>

This project is the final project of 2025 IF2130 Operating System course at [Institut Teknologi Bandung](https://itb.ac.id). Basically, we made our own x86 Protected Mode 32 bit OS from stratch, running on QEMU emulator.

The project itself divided into 4 (5) Milestones:
- Milestone 0 - Toolchain, Kernel, GDT
- Milestone 1 - Interrupt, Driver
- Milestone 2 - File System
- Milestone 3 - User Mode, Shell
- Milestone 4 - Process, Scheduler, Multitasking

Each milestone has delighting bonus for those who capable of...

<br>

<p align="center">
    <img width="300px" src="https://github.com/user-attachments/assets/2b5ffc44-0e76-4e11-9b5e-6c5ad5929fb6">
</p>
<p align="center"><i>Rawdogging 2 days debugging shell, when the error was actually in the filesystem. PS: EXT2 is a hell</i></p>

--- 

<!-- Prologue -->

### Prologue <a name="prologue"></a>

_One day, I was walking looking for my wife Firefly and when in the living room I saw Firefly sitting on the sofa and then I came over to her it turned out that Firefly was sitting relaxing in the living room while enjoying a cup of tea then I sat next to her and chatted with her._

_“My darling, what are you doing sitting here”, I asked Firefly and Firefly replied “It's okay, I'm just sitting and relaxing while drinking tea”, Firefly replied while sitting relaxed. Firefly and I both enjoyed our moments together throughout the day and I hope we will always be happy together and after that Firefly was a little tired and then leaned on my shoulder quietly and then when he finished leaning on my shoulder I then stroked his head gently with my affection for him I really enjoyed it so much that it made me fall asleep on Firefly's lap and when Firefly was stroking my head he laughed a little happily watching me fall asleep on his lap and then he whispered a soft word!_

<p align="center">
    <br>
    <img width="25%" src="https://github.com/user-attachments/assets/e131d9d1-e81d-4653-90bb-be24448dfb5b">
    <br>
</p>

_"Dear... I love you very much all the time and I promise I will be a better wife for you because I love you with all my heart, I promise I will take care of you with all my soul and I hope we will always be together like this until the end of our lives and I hope we can be happy all the time. But you need to work on your OS"._

_Fine... I'll do it. I stand up from my chair then walk to Sadang Serang camp to met all my teammates. We thriving to make the best OS we can made. Each milestone are just stepping stone for us, we break limitation of the system itself._

<p align="center">
  <em><strong>“ Mode 12h was crawling at 1fps with pixel-by-pixel torture. 640x480 planar rendering? More like 640x480 reasons to cry. Finally cracked the planar write modes - this rips through memory. Went from peasant pixels to gigachad block . Here i am standing proudly”</strong></em><br>
  <img width="30%" src="https://github.com/user-attachments/assets/783462a7-3e68-43be-9d40-9ecc9fd2bc5a"><br>
  — Kiwz
</p>

_We fought desires and laziness, we have been lobotomized and evolve more. We stand proud and we know we are the ego itself. Abolish the idea of winning by chance, and claim victory as logical sequence ringing on our mind. "We are egoist"_

_And at the end, If you're not the world's top egoist, you can never become the world's best low level engineer -idk who said this. Though we won the fight but at what cost.... Why don't i ever realize what i have until it's lost to Me? This OS is the proof that i and my teamates that have lived._

<p align="center">
    <br>
    <img width="30%" src="https://github.com/user-attachments/assets/e8f66666-9de8-4636-bbf2-211f7fe9b799">
    <br>
</p>


---

<!-- Milestones -->

### Milestones <a name="milestones"></a>

This section explains more in depth about each milestone progress, what we need to construct and etc.

<h3 align="left"> Milestone 0 </h3>

_Release Timestamp:_ 

> 18/03/2025 20:50:00 GMT+7

_Deadline:_

> 24/03/2025 21:30:00 GMT+7

Ah yes... The starting point. Who knows 4 person with big smile, curiousity, and excitement turns into Waterloo's face type shi. We're already in contact each other since January 2025 to create this team. But, yep every person have their ups and downs. Nevertheless, this is a great team to work with. Now, let's disucss the actual milestone 0 progress is.

<p align="center">
    <img width="50%" src="https://github.com/user-attachments/assets/3d632f50-fd0f-4d1c-b0b6-5f6a1244666c">
</p>

So, yeah just basic repository and installer setup with template that already created by the assistant. Using `WSL2 Ubuntu 20.04/22.04`, we only need to run these commands:

```bash
sudo apt update
sudo apt install -y nasm gcc qemu-system-x86 make genisoimage gdb
```

and

```bash
code --install-extension ms-vscode.cpptools-extension-pack
code --install-extension ms-vscode-remote.remote-wsl
```

lastly, in the root of the repository
```bash
code .
```

That's for the installing step. Then, we only need to `"copy"` the files need from book to actually run our skeleton OS.
- `kernel.c`
- `linker.id`
- `gdt.c` and `gdt.h`
- `menu.lst`
- and `Makefile`

Voila!, now we actually run it on QEMU emulator. So, what we build in Milestone 0 are:

1. Kernel
   
    -  [X] Code for the base kernel
    -  [X] Linker script for builds

2. Image Creation & Automation
   
    -  [X] Automate OS & Disk Image compilation process 
    -  [X] Automate running the OS in QEMU using `Makefile`
    
3. Global Descriptor Table

    -  [X] Segment Descriptor, GDT, & GDTR struct
    -  [X] GDT loading

<br>

<h3 align="left"> Milestone 1 </h3>

_Release Timestamp:_ 

> 05/04/2025 17:00:00 GMT+7

_Deadline:_

> 07/04/2025 21:30:00 GMT+7

Phew.. Milestone 0 was a baby (we said arrogantly, not knowing what's coming). So, in the previous milestone, we successfully run our OS BUT we can't interact. We only can look at it. Now, we need to create the interactive system so that we can actually try our OS! Thus, what we build in Milestone 1 are:

1. Framebuffer
   
    -  [X] Text display on screen with color support
    -  [X] Cursor control and positioning
    -  [X] Screen clearing functionality

2. Interrupt
   
    -  [X] Hardware interrupt handling system
    -  [X] Exception management for system stability
    -  [X] Support for device interrupt requests
    
3. Keyboard

    -  [X] Basic keyboard input processing
    -  [X] Support for special keys and modifiers (Shift & Capslock)
    -  [X] Keyboard driver activation/deactivation controls
    
4. Graphical Mode (**BONUS**)

    -  [X] VGA Mode 12h
    -  [X] Support 8 x 8 Font
    -  [X] Frame Size 640 x 480, 16-color mode

In fact, this milestone had so much help by the kit that already provides the skeleton code for what we all need :D Arigatou assistant!

<br>

<h3 align="left"> Milestone 2 </h3>

_Release Timestamp:_ 

> 05/05/2025 23:55:00 GMT+7

_Deadline:_

> 05/05/2025 23:59:00 GMT+7

No words, just hell. You know it's haram when the progress interval for this milestone is 1 month. You want memory management? ask Linus himself.

1. Disk
    -  [X] Write blocks to the storage

2. CRUD & EXT2
    -  [X] Read
    -  [X] Write
    -  [X] Delete

<br>

<h3 align="left"> Milestone 3 </h3>

_Release Timestamp:_ 

> 25/05/2025 00:40:00 GMT+7

_Deadline:_

> 19/05/2025 21:30:00 GMT+7

Do you think after Milestone 2, you get rid of touching EXT2 anymore? HELL NAH, when your shell command is not working, you know what? That's your EXT2 that has skill issue. But, anyways in Milestone 3, our OS should be able to do some commands by the user itself! So that, the user also has control to the OS!

1. Paging
   
    -  [X] Create paging data structure
    -  [X] Load the kernel at high memory address
    -  [X] Enable paging

2. User Mode
   
    -  [X] Implement file inserter for the file system
    -  [X] Create GDT entries for user mode and the Task State Segment
    -  [X] Build simple memory allocator
    -  [X] Develop simple user-mode application
    -  [X] Switch to user mode in the simple application
    
3. Shell

    -  [X] Implement system calls
    -  [X] Evolve the simple application into a shell CLI
    -  [X] Create commands for the shell
    -  [X] `cd` supports relative pathing

4. (**BONUS**)
    -  [X] `cp`, `rm`, `mv`, and `find` supports recursive implementation for non-empty folders.

<br>

<h3 align="left"> Milestone 4 </h3>

_Release Timestamp:_ 

> 29/05/2025 21:30:00 GMT+7

_Deadline:_

> 29/05/2025 21:30:00 GMT+7 (_This is actually extended from_ 26/05/2025 21:30:00 GMT+7)

Well well well.. now who have functional OS? But, we need MORE! Now, implement multitasking so that it can handle more than 1 process ^_^

1. Setting Up Process Structures

    -  [X] Create a Process Control Block (PCB) struct
    -  [X] Define the data structures required by processes

2. Implementing the Task Scheduler & Context Switching

    -  [X] Develop the task-scheduling algorithm
    -  [X] Build the context-switch mechanism

3. Shell Commands for Process Management

    -  [X] Add `exec`, `ps`, and `kill` commands for process control:

4. Running Multitasking

    -  [X] Execute clock that demonstrates running concurrency

5. (**BONUS**)

    -  [X] Bad-apple

---

### The OS <a name="os"></a>

<p align="center">
    <img width="100%" src="https://github.com/user-attachments/assets/41bb56eb-7086-4ea5-8fe8-0be5392697c3">
</p>

---

### How to Run? <a name="run"></a>

> [!NOTE]  
> Requirement depencies as mentioned in Milestone 0 before with documentation links given :D
>
> (_Don't worry, just follow the command, it will automatically install those all)_
> - [**Netwide Assembler (NASM):**](NASM-url) - An x86 assembly-language compiler that supports writing instructions directly
> - [**GNU C Compiler**](GNUC-url) - A C-language compiler used to compile code on various operating systems. 
> - [**GNU Linker (Id)**](GNUL-url) - A linker that combines object code from compilation into a single executable file. 
> - [**QEMU – System i386**](QEMU-url) - An emulator and virtual machine that runs operating systems.
> - [**GNU Make**](GNUM-url) - A tool that automates the build process in software development.
> - [**genisoimage**](iso-url) - A tool that creates disk images from an operating-system tree.
> - [**GDB (GNU Debugger)**](GDB-url) - A debugger that performs dynamic debugging on the kernel.

### Initialization

- **Clone the repository**

  ```bash
  git clone https://github.com/zirachw/Tucil3_13523004_13523098
  ```

- **Install the dependencies (Skip if already installed)**

  ```bash
  sudo apt update
  sudo apt install -y nasm gcc qemu-system-x86 make genisoimage gdb
  code --install-extension ms-vscode.cpptools-extension-pack
  code --install-extension ms-vscode-remote.remote-wsl
  ```

- Build & Run the OS!

  ```bash
  make all
  ```

> [!WARNING]  
>
> If you want keep the OS State after run and stop (Run only). Use `make run` to get the latest storage state :)

---

### Acknowledgements <a name="acknowledgements"></a>

We gratefully acknowledge:

- **Ir. Afwarman Manaf, M.Sc, Ph.D**, for expert guidance and deep dives into principal theory.
- **Ir. Robithoh Annur, M.Eng, Ph.D**, for lets us seat in to her classes.
- Our **Course Assistants**, for preparing the milestone and answering questions (Btw, thanks alot for the words in Jarkom Speficiation 😥).
- Kostan Sadang Serang with other teams, for late-night sprints.
- **Friends & Classmates**, whose spirited “just one more feature” environment kept us motivated.
- **OSDev** and the OS developer community, for creating the systematic documentation that inspired this OS.
- And **Linus Torvalds** for creating the `Linux` and `Git` himself.

<br>
<p align="center">
  <em><strong>“Those that can, do.<br>
  Those that can't, complain.”</strong></em><br>
  — Linus Torvalds
</p>

---

<h3 align="center">
Loss • © 2025 • 13523002 - 13523004 - 13523028 - 13523044
</h3>

<!-- MARKDOWN LINKS & IMAGES -->
[Asm-url]: https://docs.oracle.com/cd/E19253-01/817-5477/817-5477.pdf
[C-url]: https://devdocs.io/c/

[NASM-url]: https://www.nasm.us/
[GNUC-url]: https://man7.org/linux/man-pages/man1/gcc.1.html
[GNUL-url]: https://linux.die.net/man/1/ld
[QEMU-url]: https://www.qemu.org/docs/master/system/target-i386.html
[GNUM-url]: https://www.gnu.org/software/make/
[iso-url]: https://linux.die.net/man/1/genisoimage
[GDB-url]: https://man7.org/linux/man-pages/man1/gdb.1.html
