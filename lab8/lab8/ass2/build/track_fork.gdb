
b asm_start_process
echo [*] Breakpoint set at asm_start_process (entry of new thread)\n

b ProgramManager::fork
commands
  silent
  printf "[*] Breakpoint at ProgramManager::fork\n"
  continue
end


b ProgramManager::schedule
commands
  silent
  printf "[*] In schedule(), preparing to switch thread\n"
  continue
end


b asm_start_process
commands
  silent
  printf "[*] In asm_start_process. About to execute iret:\n"
  x/20x $esp
  info registers
  si
end

b syscall_fork
commands
  silent
  printf "[*] In syscall_fork\n"
  info registers
  continue
end

b *ProgramManager::fork
commands
  silent
  printf "[*] Returning from ProgramManager::fork, eax = "
  p $eax
  info registers
  continue
end

echo [*] GDB script loaded. Type 'c' to continue execution.\n

