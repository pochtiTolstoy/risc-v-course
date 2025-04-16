.section .text.prologue, "ax"
.globl shift_asm 

shift_asm:
  li      a5,540672 # GPIO0 base address (0x00084000)
  li      a4,1
  sw      a4,4(a5)  # Clear STCP_PIN (STCP = low)
  li      a5,0      # Init loop counter (bit_pos = 0)
  li      a2,540672
  addi    a2,a2,4   # GPIO0_CLR address
  li      a7,256    # DATA_PIN mask (1 << 8)
  li      a1,540672 # GPIO0_SET
  li      a3,1024   # SHCP_PIN mask (1 << 10)
  li      a6,8      # Loop limit
  j       .L4       # Start loop
.L2:
  sw      a7,0(a2) # Clear DATA_PIN (write to GPIO0_CLR)
.L3:
  sw      a3,0(a1)  # Set SHCP_PIN high
  sw      a3,0(a2)  # Clear SHCP_PIN low
  addi    a5,a5,1   # Inc loop counter
  beq     a5,a6,.L6 # Exit loop after loop limit
.L4:
  sra     a4,a0,a5    # shift symbol by bit_pos
  andi    a4,a4,1     # get LSB
  beq     a4,zero,.L2 # Branch if bit is 0 (put 0 in data)
  sw      a7,0(a1)    # Set DATA_PIN high
  j       .L3         # Jump to clock pulse
.L6:
  li      a5,540672 # Reload GPIO0_SET address
  li      a4,1
  sw      a4,0(a5)  # Set STCP_PIN high
  ret               # return to caller