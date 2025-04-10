In ARM Cortex-M processors, the **EXC_RETURN** value is a special value automatically loaded into the **Program Counter (PC)** when an exception (interrupt) returns. It tells the processor how to **restore the context** and **return** to the previously executing code.

### 📘 EXC_RETURN Overview:

EXC_RETURN is **loaded into PC** from **LR (Link Register)** during exception return and **must be in a specific format** (starts with `0xF`).

Example value: `0xFFFFFFFD`

---

### 📌 Format of EXC_RETURN (32-bit):

<pre class="overflow-visible!" data-start="515" data-end="818"><div class="contain-inline-size rounded-md border-[0.5px] border-token-border-medium relative bg-token-sidebar-surface-primary"><div class="flex items-center text-token-text-secondary px-4 py-2 text-xs font-sans justify-between h-9 bg-token-sidebar-surface-primary dark:bg-token-main-surface-secondary select-none rounded-t-[5px]">pgsql</div><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-sidebar-surface-primary text-token-text-secondary dark:bg-token-main-surface-secondary flex items-center rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"><button class="flex gap-1 items-center select-none px-4 py-1" aria-label="Copy"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path fill-rule="evenodd" clip-rule="evenodd" d="M7 5C7 3.34315 8.34315 2 10 2H19C20.6569 2 22 3.34315 22 5V14C22 15.6569 20.6569 17 19 17H17V19C17 20.6569 15.6569 22 14 22H5C3.34315 22 2 20.6569 2 19V10C2 8.34315 3.34315 7 5 7H7V5ZM9 7H14C15.6569 7 17 8.34315 17 10V15H19C19.5523 15 20 14.5523 20 14V5C20 4.44772 19.5523 4 19 4H10C9.44772 4 9 4.44772 9 5V7ZM5 9C4.44772 9 4 9.44772 4 10V19C4 19.5523 4.44772 20 5 20H14C14.5523 20 15 19.5523 15 19V10C15 9.44772 14.5523 9 14 9H5Z" fill="currentColor"></path></svg>Copy</button></span><span class="" data-state="closed"><button class="flex items-center gap-1 px-4 py-1 select-none"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" class="icon-xs"><path d="M2.5 5.5C4.3 5.2 5.2 4 5.5 2.5C5.8 4 6.7 5.2 8.5 5.5C6.7 5.8 5.8 7 5.5 8.5C5.2 7 4.3 5.8 2.5 5.5Z" fill="currentColor" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"></path><path d="M5.66282 16.5231L5.18413 19.3952C5.12203 19.7678 5.09098 19.9541 5.14876 20.0888C5.19933 20.2067 5.29328 20.3007 5.41118 20.3512C5.54589 20.409 5.73218 20.378 6.10476 20.3159L8.97693 19.8372C9.72813 19.712 10.1037 19.6494 10.4542 19.521C10.7652 19.407 11.0608 19.2549 11.3343 19.068C11.6425 18.8575 11.9118 18.5882 12.4503 18.0497L20 10.5C21.3807 9.11929 21.3807 6.88071 20 5.5C18.6193 4.11929 16.3807 4.11929 15 5.5L7.45026 13.0497C6.91175 13.5882 6.6425 13.8575 6.43197 14.1657C6.24513 14.4392 6.09299 14.7348 5.97903 15.0458C5.85062 15.3963 5.78802 15.7719 5.66282 16.5231Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path><path d="M14.5 7L18.5 11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path></svg>Edit</button></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre!"><span><span>Bits: </span><span>31</span><span></span><span>30</span><span>-28</span><span></span><span>27</span><span></span><span>26</span><span></span><span>25</span><span></span><span>24</span><span></span><span>23</span><span>-0</span><span>
      +</span><span>----------+------+-----+------+-----+-------------+</span><span>
      | </span><span>1</span><span></span><span>1</span><span></span><span>1</span><span></span><span>1</span><span>  | </span><span>0</span><span></span><span>0</span><span></span><span>0</span><span>|  S  |  D   |  A  |      -      |
      +</span><span>----------+------+-----+------+-----+-------------+</span><span>
        Fixed     Mode   SP   FPU?   </span><span>Return</span><span></span><span>to</span><span> Thread/</span><span>Handler</span><span>
</span></span></code></div></div></pre>

---

### 💡 Key Bits Meaning:

| Bit     | Meaning                                                                     |
| ------- | --------------------------------------------------------------------------- |
| [31:28] | Always `0b1111`→ Identifies EXC_RETURN special value.                    |
| [27]    | `S`- Stack pointer used on return:``0 = MSP, 1 = PSP               |
| [26]    | `D`- Mode to return to:``0 = Handler mode, 1 = Thread mode         |
| [25]    | `A`- Return with or without floating point context:``0 = FP active |
| [24]    | `1`- Always 1                                                             |
| [23:0]  | Reserved, always zeros                                                      |

---

### ✅ Common EXC_RETURN Values:

| Value          | Description                           |
| -------------- | ------------------------------------- |
| `0xFFFFFFF1` | Return to Handler mode, MSP, no FPU   |
| `0xFFFFFFF9` | Return to Thread mode, MSP, no FPU    |
| `0xFFFFFFFD` | Return to Thread mode, PSP, no FPU    |
| `0xFFFFFFE1` | Return to Handler mode, MSP, with FPU |
| `0xFFFFFFE9` | Return to Thread mode, MSP, with FPU  |
| `0xFFFFFFED` | Return to Thread mode, PSP, with FPU  |

---

### 🔁 Usage in Exception Return (e.g., PendSV):

When returning from an interrupt (like SysTick, PendSV):

* **CPU reads EXC_RETURN from LR**
* **Interprets the value**
* **Restores R0–R3, R12, LR, PC, xPSR**
* Optionally also R4–R11 and FPU registers if required
