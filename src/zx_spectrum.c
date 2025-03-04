#include "zx_spectrum.h"

uint8_t memory[MEM_SIZE] = { 0 };

// Precomputed parity table (even parity)
static const uint8_t parity_table[256] = {
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1
};

bool load_rom(const char* path) {
  // Existing ROM loading code
  FILE* rom = fopen(path, "rb");
  if (!rom) {
    perror("ROM load failed");
    return false;
  }

  fseek(rom, 0, SEEK_END);
  long size = ftell(rom);
  rewind(rom);

  if (size != ROM_SIZE) {
    fprintf(stderr, "Invalid Spectrum ROM: %ld bytes (expected 16KB)\n", size);
    fclose(rom);
    return false;
  }

  size_t read = fread(&memory[ROM_START], 1, ROM_SIZE, rom);
  fclose(rom);

  if (read != ROM_SIZE) {
    fprintf(stderr, "Partial ROM read: %zu/16384 bytes\n", read);
    return false;
  }

  printf("Loaded Spectrum ROM successfully\n");
  return true;
}

bool load_z80_snapshot(const char* filename, Z80_State* state) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    perror("Failed to open Z80 file");
    return false;
  }

  // Read main header (at least 30 bytes)
  uint8_t header[58];
  if (fread(header, 1, 30, file) != 30) {
    fclose(file);
    return false;
  }

  // Detect version (byte 29/0x1D)
  int version = 1;
  if (header[29] == 23 || header[29] == 54) version = 2;

  // Read extended header for version 2/3
  if (version >= 2) {
    if (fread(&header[30], 1, 28, file) != 28) {
      fclose(file);
      return false;
    }
  }

  // Handle different version formats
  uint16_t pc = (header[7] << 8) | header[6];
  bool compressed = false;
  size_t data_offset = 30;

  if (version >= 2) {
    // Version 2+ specific handling
    compressed = (header[34] & 0x20); // Compression flag in extended header
    data_offset = 32 + header[30];    // Skip additional headers
    pc = (header[33] << 8) | header[32];
  }
  else {
    // Original version 1 handling
    compressed = (header[12] & 0x20);
  }

  // Read register state
  state->pc = pc;
  state->af = (header[0] << 8) | header[1];
  state->bc = (header[2] << 8) | header[3];
  state->de = (header[4] << 8) | header[5];
  state->hl = (header[8] << 8) | header[9];
  state->ix = (header[10] << 8) | header[11];
  state->iy = (header[12] << 8) | header[13];
  state->sp = (header[14] << 8) | header[15];
  state->f = header[16];
  state->imode = header[17];
  state->r = header[18];

  // Get RAM data
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, data_offset, SEEK_SET);
  long data_size = file_size - data_offset;

  uint8_t* data = malloc(data_size);
  if (!data || fread(data, 1, data_size, file) != data_size) {
    free(data);
    fclose(file);
    return false;
  }
  fclose(file);

  // Handle different memory models
  size_t dest_idx = 0x4000;  // Standard 48K Spectrum RAM start
  size_t ram_size = 0xC000;   // 48K RAM size

  if (!compressed) {
    // Simple memory copy for uncompressed data
    if (data_size != ram_size) {
      fprintf(stderr, "Unexpected RAM size: %ld (expected %zu)\n",
        data_size, ram_size);
      free(data);
      return false;
    }
    memcpy(&memory[dest_idx], data, ram_size);
  }
  else {
    // Enhanced decompression handling
    size_t src_idx = 0;
    while (src_idx < data_size && dest_idx < (0x4000 + ram_size)) {
      if (data[src_idx] == 0xED && (src_idx + 1 < data_size) && data[src_idx + 1] == 0xED) {
        // Compressed block
        src_idx += 2;
        if (src_idx + 1 >= data_size) break;

        uint16_t count = data[src_idx++];
        uint16_t value = data[src_idx++];

        if (count == 0) count = 256;
        memset(&memory[dest_idx], value, count);
        dest_idx += count;
      }
      else {
        // Uncompressed byte
        memory[dest_idx++] = data[src_idx++];
      }
    }
  }

  free(data);
  printf("Successfully loaded Z80 snapshot (version %d)\n", version);
  return true;
}

uint8_t mem_read(uint32_t addr) {
  return memory[addr];
}

uint16_t mem_read16(uint32_t addr) {
  return (memory[addr + 1] << 8) | memory[addr];
}

void mem_write(uint32_t addr, uint8_t value) {
  if (addr >= MEM_SIZE) return;
  memory[addr] = value;
}

void mem_write16(uint32_t addr, uint8_t value) {
  if (addr >= MEM_SIZE) return;
  memory[addr + 1] = value >> 8;
  memory[addr] = value & 0xFF;
}

uint8_t input_port(Z80_State* state, uint8_t port) {
  return mem_read(port);
}

void output_port(Z80_State* state, uint8_t port, uint8_t val) {
  // Set the border color
  memory[0x5800 + (port & 0x1F)] = val;
  //printf("Port %02X: %02X\n", port, val);
}

void z80_init(Z80_State* state) {
  state->pc = 0x0000;
  state->sp = 0xFFFF;
  state->iff1 = state->iff2 = 0;
  state->imode = 0;
}

static void add_a(Z80_State* state, uint8_t val) {
  uint16_t res = state->a + val;
  uint8_t carry = (res > 0xFF) ? FLAG_C : 0;

  CLR_FLAG(state, FLAG_N | FLAG_C | FLAG_H);
  SET_FLAG(state, carry);
  if (((state->a & 0x0F) + (val & 0x0F)) > 0x0F) SET_FLAG(state, FLAG_H);

  state->a = res & 0xFF;
  UPDATE_SZP(state, state->a);
}

static void sub_a(Z80_State* state, uint8_t val) {
  uint16_t res = state->a - val;
  uint8_t carry = (res > 0xFF) ? FLAG_C : 0;

  SET_FLAG(state, FLAG_N);
  CLR_FLAG(state, FLAG_C | FLAG_H);
  SET_FLAG(state, carry);
  if (((state->a & 0x0F) - (val & 0x0F)) & 0x10) SET_FLAG(state, FLAG_H);

  state->a = res & 0xFF;
  UPDATE_SZP(state, state->a);
}

int decode_cb(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);
  uint8_t temp;

  switch (opcode) {

  case 0x00: // RLC B
    temp = (state->b >> 7) & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->b = (state->b << 1) | temp;
    UPDATE_SZP(state, state->b);
    break;

  case 0x01: // RLC C
    temp = (state->c >> 7) & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->c = (state->c << 1) | temp;
    UPDATE_SZP(state, state->c);
    break;

  case 0x02: // RLC D
    temp = (state->d >> 7) & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->d = (state->d << 1) | temp;
    UPDATE_SZP(state, state->d);
    break;

  case 0x03: // RLC E
    temp = (state->e >> 7) & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->e = (state->e << 1) | temp;
    UPDATE_SZP(state, state->e);
    break;

  case 0x04: // RLC H
    temp = (state->h >> 7) & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->h = (state->h << 1) | temp;
    UPDATE_SZP(state, state->h);
    break;

  case 0x05: // RLC L
    temp = (state->l >> 7) & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->l = (state->l << 1) | temp;
    UPDATE_SZP(state, state->l);
    break;

  case 0x06: // RLC (HL)
    temp = (mem_read(state->hl) >> 7) & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    mem_write(state->hl, (mem_read(state->hl) << 1) | temp);
    UPDATE_SZP(state, mem_read(state->hl));
    break;

  case 0x07: // RLC A
    temp = (state->a >> 7) & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->a = (state->a << 1) | temp;
    UPDATE_SZP(state, state->a);
    break;

  case 0x08: // RRC B
    temp = state->b & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->b = (state->b >> 1) | (temp << 7);
    UPDATE_SZP(state, state->b);
    break;

  case 0x09: // RRC C
    temp = state->c & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->c = (state->c >> 1) | (temp << 7);
    UPDATE_SZP(state, state->c);
    break;

  case 0x0A: // RRC D
    temp = state->d & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->d = (state->d >> 1) | (temp << 7);
    UPDATE_SZP(state, state->d);
    break;

  case 0x0B: // RRC E
    temp = state->e & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->e = (state->e >> 1) | (temp << 7);
    UPDATE_SZP(state, state->e);
    break;

  case 0x0C: // RRC H
    temp = state->h & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->h = (state->h >> 1) | (temp << 7);
    UPDATE_SZP(state, state->h);
    break;

  case 0x0D: // RRC L
    temp = state->l & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->l = (state->l >> 1) | (temp << 7);
    UPDATE_SZP(state, state->l);
    break;

  case 0x0E: // RRC (HL)
    temp = mem_read(state->hl) & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    mem_write(state->hl, (mem_read(state->hl) >> 1) | (temp << 7));
    UPDATE_SZP(state, mem_read(state->hl));
    break;

  case 0x0F: // RRC A
    temp = state->a & 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    state->a = (state->a >> 1) | (temp << 7);
    UPDATE_SZP(state, state->a);
    break;

  case 0x10: // RL B
    temp = (state->b << 1) | (state->f & FLAG_C);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp & 0x100) ? FLAG_C : 0;
    state->b = temp & 0xFF;
    UPDATE_SZP(state, state->b);
    break;

  case 0x11: // RL C
    temp = (state->c << 1) | (state->f & FLAG_C);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp & 0x100) ? FLAG_C : 0;
    state->c = temp & 0xFF;
    UPDATE_SZP(state, state->c);
    break;

  case 0x12: // RL D
    temp = (state->d << 1) | (state->f & FLAG_C);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp & 0x100) ? FLAG_C : 0;
    state->d = temp & 0xFF;
    UPDATE_SZP(state, state->d);
    break;

  case 0x13: // RL E
    temp = (state->e << 1) | (state->f & FLAG_C);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp & 0x100) ? FLAG_C : 0;
    state->e = temp & 0xFF;
    UPDATE_SZP(state, state->e);
    break;

  case 0x14: // RL H
    temp = (state->h << 1) | (state->f & FLAG_C);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp & 0x100) ? FLAG_C : 0;
    state->h = temp & 0xFF;
    UPDATE_SZP(state, state->h);
    break;

  case 0x15: // RL L
    temp = (state->l << 1) | (state->f & FLAG_C);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp & 0x100) ? FLAG_C : 0;
    state->l = temp & 0xFF;
    UPDATE_SZP(state, state->l);
    break;

  case 0x16: // RL (HL)
    temp = (mem_read(state->hl) << 1) | (state->f & FLAG_C);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp & 0x100) ? FLAG_C : 0;
    mem_write(state->hl, temp & 0xFF);
    UPDATE_SZP(state, mem_read(state->hl));
    break;

  case 0x17: // RL A
    temp = (state->a << 1) | (state->f & FLAG_C);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp & 0x100) ? FLAG_C : 0;
    state->a = temp & 0xFF;
    UPDATE_SZP(state, state->a);
    break;

  case 0x18: // RR B
    temp = state->b & 1;
    state->b = (state->b >> 1) | ((state->f & FLAG_C) << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->b);
    break;

  case 0x19: // RR C
    temp = state->c & 1;
    state->c = (state->c >> 1) | ((state->f & FLAG_C) << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->c);
    break;

  case 0x1A: // RR D
    temp = state->d & 1;
    state->d = (state->d >> 1) | ((state->f & FLAG_C) << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->d);
    break;

  case 0x1B: // RR E
    temp = state->e & 1;
    state->e = (state->e >> 1) | ((state->f & FLAG_C) << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->e);
    break;

  case 0x1C: // RR H
    temp = state->h & 1;
    state->h = (state->h >> 1) | ((state->f & FLAG_C) << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->h);
    break;

  case 0x1D: // RR L
    temp = state->l & 1;
    state->l = (state->l >> 1) | ((state->f & FLAG_C) << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->l);
    break;

  case 0x1E: // RR (HL)
    temp = mem_read(state->hl) & 1;
    mem_write(state->hl, (mem_read(state->hl) >> 1) | ((state->f & FLAG_C) << 7));
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, mem_read(state->hl));
    break;

  case 0x1F: // RR A
    temp = state->a & 1;
    state->a = (state->a >> 1) | ((state->f & FLAG_C) << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->a);
    break;

  case 0x20: // SLA B
    temp = (state->b >> 7) & 1;
    state->b = state->b << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->b);
    break;

  case 0x21: // SLA C
    temp = (state->c >> 7) & 1;
    state->c = state->c << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->c);
    break;

  case 0x22: // SLA D
    temp = (state->d >> 7) & 1;
    state->d = state->d << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->d);
    break;

  case 0x23: // SLA E
    temp = (state->e >> 7) & 1;
    state->e = state->e << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->e);
    break;

  case 0x24: // SLA H
    temp = (state->h >> 7) & 1;
    state->h = state->h << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->h);
    break;

  case 0x25: // SLA L
    temp = (state->l >> 7) & 1;
    state->l = state->l << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->l);
    break;

  case 0x26: // SLA (HL)
    temp = (mem_read(state->hl) >> 7) & 1;
    mem_write(state->hl, mem_read(state->hl) << 1);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, mem_read(state->hl));
    break;

  case 0x27: // SLA A
    temp = (state->a >> 7) & 1;
    state->a = state->a << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->a);

  case 0x28: // SRA B
    temp = state->b & 1;
    state->b = (state->b >> 1) | (state->b & 0x80);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->b);
    break;

  case 0x29: // SRA C
    temp = state->c & 1;
    state->c = (state->c >> 1) | (state->c & 0x80);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->c);
    break;

  case 0x2A: // SRA D
    temp = state->d & 1;
    state->d = (state->d >> 1) | (state->d & 0x80);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->d);
    break;

  case 0x2B: // SRA E
    temp = state->e & 1;
    state->e = (state->e >> 1) | (state->e & 0x80);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->e);
    break;

  case 0x2C: // SRA H
    temp = state->h & 1;
    state->h = (state->h >> 1) | (state->h & 0x80);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->h);
    break;

  case 0x2D: // SRA L
    temp = state->l & 1;
    state->l = (state->l >> 1) | (state->l & 0x80);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->l);
    break;

  case 0x2E: // SRA (HL)
    temp = mem_read(state->hl) & 1;
    uint8_t val = mem_read(state->hl);
    mem_write(state->hl, (val >> 1) | (val & 0x80));
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, mem_read(state->hl));
    break;

  case 0x2F: // SRA A
    temp = state->a & 1;
    state->a = (state->a >> 1) | (state->a & 0x80);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->a);
    break;

  case 0x30: // SLL B
    temp = state->b >> 7;
    state->b = (state->b << 1) & 0xFF;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->b);
    break;

  case 0x31: // SLL C
    temp = state->c >> 7;
    state->c = (state->c << 1) & 0xFF;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->c);
    break;

  case 0x32: // SLL D
    temp = state->d >> 7;
    state->d = (state->d << 1) & 0xFF;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->d);
    break;

  case 0x33: // SLL E
    temp = state->e >> 7;
    state->e = (state->e << 1) & 0xFF;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->e);
    break;

  case 0x34: // SLL H
    temp = state->h >> 7;
    state->h = (state->h << 1) & 0xFF;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->h);
    break;

  case 0x35: // SLL L
    temp = state->l >> 7;
    state->l = (state->l << 1) & 0xFF;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->l);
    break;

  case 0x36: // SLL (HL)
    temp = mem_read(state->hl) >> 7;
    val = mem_read(state->hl);
    mem_write(state->hl, (val << 1) & 0xFF);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, mem_read(state->hl));
    break;

  case 0x37: // SLL A
    temp = state->a >> 7;
    state->a = (state->a << 1) & 0xFF;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->a);
    break;

  case 0x38: // SRL B
    temp = state->b & 1;
    state->b >>= 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->b);
    break;

  case 0x39: // SRL C
    temp = state->c & 1;
    state->c >>= 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->c);
    break;

  case 0x3A: // SRL D
    temp = state->d & 1;
    state->d >>= 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->d);
    break;

  case 0x3B: // SRL E
    temp = state->e & 1;
    state->e >>= 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->e);
    break;

  case 0x3C: // SRL H
    temp = state->h & 1;
    state->h >>= 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->h);
    break;

  case 0x3D: // SRL L
    temp = state->l & 1;
    state->l >>= 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->l);
    break;

  case 0x3E: // SRL (HL)
    temp = mem_read(state->hl) & 1;
    val = mem_read(state->hl) >> 1;
    mem_write(state->hl, val);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, val);
    break;

  case 0x3F: // SRL A
    temp = state->a & 1;
    state->a >>= 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= temp ? FLAG_C : 0;
    UPDATE_SZP(state, state->a);
    break;

  case 0x40: // BIT 0, B
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->b & 0x01) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x41: // BIT 0, C
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->c & 0x01) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x42: // BIT 0, D
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->d & 0x01) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x43: // BIT 0, E
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->e & 0x01) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x44: // BIT 0, H
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->h & 0x01) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x45: // BIT 0, L
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->l & 0x01) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x46: // BIT 0, (HL)
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (mem_read(state->hl) & 0x01) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x47: // BIT 0, A
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->a & 0x01) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x48: // BIT 1, B
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->b & 0x02) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x49: // BIT 1, C
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->c & 0x02) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x4A: // BIT 1, D
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->d & 0x02) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x4B: // BIT 1, E
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->e & 0x02) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x4C: // BIT 1, H
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->h & 0x02) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x4D: // BIT 1, L
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->l & 0x02) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x4E: // BIT 1, (HL)
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (mem_read(state->hl) & 0x02) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x4F: // BIT 1, A
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->a & 0x02) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x50: // BIT 2, B
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->b & 0x04) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x51: // BIT 2, C
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->c & 0x04) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x52: // BIT 2, D
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->d & 0x04) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x53: // BIT 2, E
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->e & 0x04) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x54: // BIT 2, H
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->h & 0x04) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x55: // BIT 2, L
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->l & 0x04) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x56: // BIT 2, (HL)
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (mem_read(state->hl) & 0x04) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x57: // BIT 2, A
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->a & 0x04) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x58: // BIT 3, B
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->b & 0x08) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x59: // BIT 3, C
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->c & 0x08) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x5A: // BIT 3, D
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->d & 0x08) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x5B: // BIT 3, E
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->e & 0x08) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x5C: // BIT 3, H
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->h & 0x08) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x5D: // BIT 3, L
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->l & 0x08) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x5E: // BIT 3, (HL)
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (mem_read(state->hl) & 0x08) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x5F: // BIT 3, A
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->a & 0x08) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x60: // BIT 4, B
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->b & 0x10) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x61: // BIT 4, C
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->c & 0x10) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x62: // BIT 4, D
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->d & 0x10) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x63: // BIT 4, E
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->e & 0x10) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x64: // BIT 4, H
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->h & 0x10) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x65: // BIT 4, L
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->l & 0x10) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x66: // BIT 4, (HL)
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (mem_read(state->hl) & 0x10) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x67: // BIT 4, A
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->a & 0x10) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x68: // BIT 5, B
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->b & 0x20) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x69: // BIT 5, C
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->c & 0x20) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x6a: // BIT 5, D
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->d & 0x20) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x6b: // BIT 5, E
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->e & 0x20) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x6c: // BIT 5, H
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->h & 0x20) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x6d: // BIT 5, L
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->l & 0x20) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x6e: // BIT 5, (HL)
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (mem_read(state->hl) & 0x20) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x6f: // BIT 5, A
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->a & 0x20) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x70: // BIT 6, B
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->b & 0x40) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x71: // BIT 6, C
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->c & 0x40) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x72: // BIT 6, D
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->d & 0x40) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x73: // BIT 6, E
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->e & 0x40) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x74: // BIT 6, H
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->h & 0x40) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x75: // BIT 6, L
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->l & 0x40) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x76: // BIT 6, (HL)
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (mem_read(state->hl) & 0x40) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x77: // BIT 6, A
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->a & 0x40) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x78: // BIT 7, B
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->b & 0x80) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x79: // BIT 7, C
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->c & 0x80) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x7a: // BIT 7, D
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->d & 0x80) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x7b: // BIT 7, E
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->e & 0x80) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x7c: // BIT 7, H
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->h & 0x80) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x7d: // BIT 7, L
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->l & 0x80) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x7e: // BIT 7, (HL)
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (mem_read(state->hl) & 0x80) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x7f: // BIT 7, A
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H);
    state->f |= (state->a & 0x80) ? 0 : FLAG_Z;
    state->f |= FLAG_H;
    break;

  case 0x80: // RES 0,B
    state->b &= ~(1 << 0);
    break;

  case 0x81: // RES 0,C
    state->c &= ~(1 << 0);
    break;

  case 0x82: // RES 0,D
    state->d &= ~(1 << 0);
    break;

  case 0x83: // RES 0,E
    state->e &= ~(1 << 0);
    break;

  case 0x84: // RES 0,H
    state->h &= ~(1 << 0);
    break;

  case 0x85: // RES 0,L
    state->l &= ~(1 << 0);
    break;

  case 0x86: // RES 0,(HL)
    mem_write(state->hl, mem_read(state->hl) & ~(1 << 0));
    break;

  case 0x87: // RES 0,A
    state->a &= ~(1 << 0);
    break;

  case 0x88: // RES 1,B
    state->b &= ~(1 << 1);
    break;

  case 0x89: // RES 1,C
    state->c &= ~(1 << 1);
    break;

  case 0x8a: // RES 1,D
    state->d &= ~(1 << 1);
    break;

  case 0x8b: // RES 1,E
    state->e &= ~(1 << 1);
    break;

  case 0x8c: // RES 1,H
    state->h &= ~(1 << 1);
    break;

  case 0x8d: // RES 1,L
    state->l &= ~(1 << 1);
    break;

  case 0x8e: // RES 1,(HL)
    mem_write(state->hl, mem_read(state->hl) & ~(1 << 1));
    break;

  case 0x8f: // RES 1,A
    state->a &= ~(1 << 1);
    break;

  case 0x90: // RES 2,B
    state->b &= ~(1 << 2);
    break;

  case 0x91: // RES 2,C
    state->c &= ~(1 << 2);
    break;

  case 0x92: // RES 2,D
    state->d &= ~(1 << 2);
    break;

  case 0x93: // RES 2,E
    state->e &= ~(1 << 2);
    break;

  case 0x94: // RES 2,H
    state->h &= ~(1 << 2);
    break;

  case 0x95: // RES 2,L
    state->l &= ~(1 << 2);
    break;

  case 0x96: // RES 2,(HL)
    mem_write(state->hl, mem_read(state->hl) & ~(1 << 2));
    break;

  case 0x97: // RES 2,A
    state->a &= ~(1 << 2);
    break;

  case 0x98: // RES 3,B
    state->b &= ~(1 << 3);
    break;

  case 0x99: // RES 3,C
    state->c &= ~(1 << 3);
    break;

  case 0x9a: // RES 3,D
    state->d &= ~(1 << 3);
    break;

  case 0x9b: // RES 3,E
    state->e &= ~(1 << 3);
    break;

  case 0x9c: // RES 3,H
    state->h &= ~(1 << 3);
    break;

  case 0x9d: // RES 3,L
    state->l &= ~(1 << 3);
    break;

  case 0x9e: // RES 3,(HL)
    mem_write(state->hl, mem_read(state->hl) & ~(1 << 3));
    break;

  case 0x9f: // RES 3,A
    state->a &= ~(1 << 3);
    break;

  case 0xa0: // RES 4,B
    state->b &= ~(1 << 4);
    break;

  case 0xa1: // RES 4,C
    state->c &= ~(1 << 4);
    break;

  case 0xa2: // RES 4,D
    state->d &= ~(1 << 4);
    break;

  case 0xa3: // RES 4,E
    state->e &= ~(1 << 4);
    break;

  case 0xa4: // RES 4,H
    state->h &= ~(1 << 4);
    break;

  case 0xa5: // RES 4,L
    state->l &= ~(1 << 4);
    break;

  case 0xa6: // RES 4,(HL)
    mem_write(state->hl, mem_read(state->hl) & ~(1 << 4));
    break;

  case 0xa7: // RES 4,A
    state->a &= ~(1 << 4);
    break;

  case 0xa8: // RES 5,B
    state->b &= ~(1 << 5);
    break;

  case 0xa9: // RES 5,C
    state->c &= ~(1 << 5);
    break;

  case 0xaa: // RES 5,D
    state->d &= ~(1 << 5);
    break;

  case 0xab: // RES 5,E
    state->e &= ~(1 << 5);
    break;

  case 0xac: // RES 5,H
    state->h &= ~(1 << 5);
    break;

  case 0xad: // RES 5,L
    state->l &= ~(1 << 5);
    break;

  case 0xae: // RES 5,(HL)
    mem_write(state->hl, mem_read(state->hl) & ~(1 << 5));
    break;

  case 0xaf: // RES 5,A
    state->a &= ~(1 << 5);
    break;

  case 0xb0: // RES 6,B
    state->b &= ~(1 << 6);
    break;

  case 0xb1: // RES 6,C
    state->c &= ~(1 << 6);
    break;

  case 0xb2: // RES 6,D
    state->d &= ~(1 << 6);
    break;

  case 0xb3: // RES 6,E
    state->e &= ~(1 << 6);
    break;

  case 0xb4: // RES 6,H
    state->h &= ~(1 << 6);
    break;

  case 0xb5: // RES 6,L
    state->l &= ~(1 << 6);
    break;

  case 0xb6: // RES 6,(HL)
    mem_write(state->hl, mem_read(state->hl) & ~(1 << 6));
    break;

  case 0xb7: // RES 6,A
    state->a &= ~(1 << 6);
    break;

  case 0xb8: // RES 7,B
    state->b &= ~(1 << 7);
    break;

  case 0xb9: // RES 7,C
    state->c &= ~(1 << 7);
    break;

  case 0xba: // RES 7,D
    state->d &= ~(1 << 7);
    break;

  case 0xbb: // RES 7,E
    state->e &= ~(1 << 7);
    break;

  case 0xbc: // RES 7,H
    state->h &= ~(1 << 7);
    break;

  case 0xbd: // RES 7,L
    state->l &= ~(1 << 7);
    break;

  case 0xbe: // RES 7,(HL)
    mem_write(state->hl, mem_read(state->hl) & ~(1 << 7));
    break;

  case 0xbf: // RES 7,A
    state->a &= ~(1 << 7);
    break;

  case 0xc0: // SET 0,B
    state->b |= (1 << 0);
    break;

  case 0xc1: // SET 0,C
    state->c |= (1 << 0);
    break;

  case 0xc2: // SET 0,D
    state->d |= (1 << 0);
    break;

  case 0xc3: // SET 0,E
    state->e |= (1 << 0);
    break;

  case 0xc4: // SET 0,H
    state->h |= (1 << 0);
    break;

  case 0xc5: // SET 0,L
    state->l |= (1 << 0);
    break;

  case 0xc6: // SET 0,(HL)
    mem_write(state->hl, mem_read(state->hl) | (1 << 0));
    break;

  case 0xc7: // SET 0,A
    state->a |= (1 << 0);
    break;

  case 0xc8: // SET 1,B
    state->b |= (1 << 1);
    break;

  case 0xc9: // SET 1,C
    state->c |= (1 << 1);
    break;

  case 0xca: // SET 1,D
    state->d |= (1 << 1);
    break;

  case 0xcb: // SET 1,E
    state->e |= (1 << 1);
    break;

  case 0xcc: // SET 1,H
    state->h |= (1 << 1);
    break;

  case 0xcd: // SET 1,L
    state->l |= (1 << 1);
    break;

  case 0xce: // SET 1,(HL)
    mem_write(state->hl, mem_read(state->hl) | (1 << 1));
    break;

  case 0xcf: // SET 1,A
    state->a |= (1 << 1);
    break;

  case 0xd0: // SET 2,B
    state->b |= (1 << 2);
    break;

  case 0xd1: // SET 2,C
    state->c |= (1 << 2);
    break;

  case 0xd2: // SET 2,D
    state->d |= (1 << 2);
    break;

  case 0xd3: // SET 2,E
    state->e |= (1 << 2);
    break;

  case 0xd4: // SET 2,H
    state->h |= (1 << 2);
    break;

  case 0xd5: // SET 2,L
    state->l |= (1 << 2);
    break;

  case 0xd6: // SET 2,(HL)
    mem_write(state->hl, mem_read(state->hl) | (1 << 2));
    break;

  case 0xd7: // SET 2,A
    state->a |= (1 << 2);
    break;

  case 0xd8: // SET 3,B
    state->b |= (1 << 3);
    break;

  case 0xd9: // SET 3,C
    state->c |= (1 << 3);
    break;

  case 0xda: // SET 3,D
    state->d |= (1 << 3);
    break;

  case 0xdb: // SET 3,E
    state->e |= (1 << 3);
    break;

  case 0xdc: // SET 3,H
    state->h |= (1 << 3);
    break;

  case 0xdd: // SET 3,L
    state->l |= (1 << 3);
    break;

  case 0xde: // SET 3,(HL)
    mem_write(state->hl, mem_read(state->hl) | (1 << 3));
    break;

  case 0xdf: // SET 3,A
    state->a |= (1 << 3);
    break;

  case 0xe0: // SET 4,B
    state->b |= (1 << 4);
    break;

  case 0xe1: // SET 4,C
    state->c |= (1 << 4);
    break;

  case 0xe2: // SET 4,D
    state->d |= (1 << 4);
    break;

  case 0xe3: // SET 4,E
    state->e |= (1 << 4);
    break;

  case 0xe4: // SET 4,H
    state->h |= (1 << 4);
    break;

  case 0xe5: // SET 4,L
    state->l |= (1 << 4);
    break;

  case 0xe6: // SET 4,(HL)
    mem_write(state->hl, mem_read(state->hl) | (1 << 4));
    break;

  case 0xe7: // SET 4,A
    state->a |= (1 << 4);
    break;

  case 0xe8: // SET 5,B
    state->b |= (1 << 5);
    break;

  case 0xe9: // SET 5,C
    state->c |= (1 << 5);
    break;

  case 0xea: // SET 5,D
    state->d |= (1 << 5);
    break;

  case 0xeb: // SET 5,E
    state->e |= (1 << 5);
    break;

  case 0xec: // SET 5,H
    state->h |= (1 << 5);
    break;

  case 0xed: // SET 5,L
    state->l |= (1 << 5);
    break;

  case 0xee: // SET 5,(HL)
    mem_write(state->hl, mem_read(state->hl) | (1 << 5));
    break;

  case 0xef: // SET 5,A
    state->a |= (1 << 5);
    break;

  case 0xf0: // SET 6,B
    state->b |= (1 << 6);
    break;

  case 0xf1: // SET 6,C
    state->c |= (1 << 6);
    break;

  case 0xf2: // SET 6,D
    state->d |= (1 << 6);
    break;

  case 0xf3: // SET 6,E
    state->e |= (1 << 6);
    break;

  case 0xf4: // SET 6,H
    state->h |= (1 << 6);
    break;

  case 0xf5: // SET 6,L
    state->l |= (1 << 6);
    break;

  case 0xf6: // SET 6,(HL)
    mem_write(state->hl, mem_read(state->hl) | (1 << 6));
    break;

  case 0xf7: // SET 6,A
    state->a |= (1 << 6);
    break;

  case 0xf8: // SET 7,B
    state->b |= (1 << 7);
    break;

  case 0xf9: // SET 7,C
    state->c |= (1 << 7);
    break;

  case 0xfa: // SET 7,D
    state->d |= (1 << 7);
    break;

  case 0xfb: // SET 7,E
    state->e |= (1 << 7);
    break;

  case 0xfc: // SET 7,H
    state->h |= (1 << 7);
    break;

  case 0xfd: // SET 7,L
    state->l |= (1 << 7);
    break;

  case 0xfe: // SET 7,(HL)
    mem_write(state->hl, mem_read(state->hl) | (1 << 7));
    break;

  case 0xff: // SET 7,A
    state->a |= (1 << 7);
    break;

  default:
    printf("Unknown CB opcode: %02X\n", opcode);
    return 0;
  }
}

int decode_dd(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);
  uint8_t temp;
  uint16_t temp16;
  uint8_t n;
  uint8_t carry;
  uint8_t res;
  uint8_t port;

  switch (opcode) {

  case 0x09: // ADD IX,BC
    state->ix += state->bc;
    break;

  case 0x19: // ADD IX,DE
    state->ix += state->de;
    break;

  case 0x21: // LD IX,nnnn
    state->ix = (mem_read(state->pc++) | (mem_read(state->pc++) << 8));
    break;

  case 0x22: // LD (nnnn),IX
    temp16 = (mem_read(state->pc++) | (mem_read(state->pc++) << 8));
    mem_write16(temp16, state->ix);
    break;

  case 0x23: // INC IX
    state->ix++;
    break;

  case 0x24: // INC IXH
    state->ix = (state->ix & 0x00FF) | ((state->ix + 0x0100) & 0xFF00);
    break;

  case 0x25: // DEC IXH
    state->ix = (state->ix & 0x00FF) | ((state->ix - 0x0100) & 0xFF00);
    break;

  case 0x26: // LD IXH,nn
    state->ix = (state->ix & 0x00FF) | (mem_read(state->pc++) << 8);
    break;

  case 0x29: // ADD IX,IX
    state->ix += state->ix;
    break;

  case 0x2A: // LD IX,(nnnn)
    temp16 = (mem_read(state->pc++) | (mem_read(state->pc++) << 8));
    state->ix = mem_read16(temp16);
    break;

  case 0x2B: // DEC IX
    state->ix--;
    break;

  case 0x2C: // INC IXL
    state->ix = (state->ix & 0xFF00) | ((state->ix + 0x0001) & 0x00FF);
    break;

  case 0x2D: // DEC IXL
    state->ix = (state->ix & 0xFF00) | ((state->ix - 0x0001) & 0x00FF);
    break;

  case 0x2E: // LD IXL,nn
    state->ix = (state->ix & 0xFF00) | mem_read(state->pc++);
    break;

  case 0x34: // INC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, mem_read(temp16) + 1);
    break;

  case 0x35: // DEC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, mem_read(temp16) - 1);
    break;

  case 0x36: // LD (IX+dd),nn
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, mem_read(state->pc++));
    break;

  case 0x39: // ADD IX,SP
    state->ix += state->sp;
    break;

  case 0x44: // LD B,IXH
    state->b = (state->ix >> 8) & 0xFF;
    break;

  case 0x45: // LD B,IXL
    state->b = state->ix & 0xFF;
    break;

  case 0x46: // LD B,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->b = mem_read(temp16);
    break;

  case 0x4C: // LD C,IXH
    state->c = (state->ix >> 8) & 0xFF;
    break;

  case 0x4D: // LD C,IXL
    state->c = state->ix & 0xFF;
    break;

  case 0x4E: // LD C,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->c = mem_read(temp16);
    break;

  case 0x54: // LD D,IXH
    state->d = (state->ix >> 8) & 0xFF;
    break;

  case 0x55: // LD D,IXL
    state->d = state->ix & 0xFF;
    break;

  case 0x56: // LD D,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->d = mem_read(temp16);
    break;

  case 0x5C: // LD E,IXH
    state->e = (state->ix >> 8) & 0xFF;
    break;

  case 0x5D: // LD E,IXL
    state->e = state->ix & 0xFF;
    break;

  case 0x5E: // LD E,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->e = mem_read(temp16);
    break;

  case 0x60: // LD IXH,B
    state->ix = (state->ix & 0x00FF) | (state->b << 8);
    break;

  case 0x61: // LD IXH,C
    state->ix = (state->ix & 0x00FF) | (state->c << 8);
    break;

  case 0x62: // LD IXH,D
    state->ix = (state->ix & 0x00FF) | (state->d << 8);
    break;

  case 0x63: // LD IXH,E
    state->ix = (state->ix & 0x00FF) | (state->e << 8);
    break;

  case 0x64: // LD IXH,IXH
    // No operation needed, IXH already holds its value
    break;

  case 0x65: // LD IXH,IXL
    state->ix = (state->ix & 0x00FF) | ((state->ix & 0xFF) << 8);
    break;

  case 0x66: // LD H,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->h = mem_read(temp16);
    break;

  case 0x67: // LD IXH,A
    state->ix = (state->ix & 0x00FF) | (state->a << 8);
    break;

  case 0x68: // LD IXL,B
    state->ix = (state->ix & 0xFF00) | state->b;
    break;

  case 0x69: // LD IXL,C
    state->ix = (state->ix & 0xFF00) | state->c;
    break;

  case 0x6A: // LD IXL,D
    state->ix = (state->ix & 0xFF00) | state->d;
    break;

  case 0x6B: // LD IXL,E
    state->ix = (state->ix & 0xFF00) | state->e;
    break;

  case 0x6C: // LD IXL,IXH
    state->ix = (state->ix & 0xFF00) | ((state->ix >> 8) & 0xFF);
    break;

  case 0x6D: // LD IXL,IXL
    // No operation needed, IXL already holds its value
    break;

  case 0x6E: // LD L,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->l = mem_read(temp16);
    break;

  case 0x6F: // LD IXL,A
    state->ix = (state->ix & 0xFF00) | state->a;
    break;

  case 0x70: // LD (IX+dd),B
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, state->b);
    break;

  case 0x71: // LD (IX+dd),C
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, state->c);
    break;

  case 0x72: // LD (IX+dd),D
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, state->d);
    break;

  case 0x73: // LD (IX+dd),E
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, state->e);
    break;

  case 0x74: // LD (IX+dd),H
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, state->h);
    break;

  case 0x75: // LD (IX+dd),L
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, state->l);
    break;

  case 0x77: // LD (IX+dd),A
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    mem_write(temp16, state->a);
    break;

  case 0x7C: // LD A,IXH
    state->a = (state->ix >> 8) & 0xFF;
    break;

  case 0x7D: // LD A,IXL
    state->a = state->ix & 0xFF;
    break;

  case 0x7E: // LD A,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->a = mem_read(temp16);
    break;

  case 0x84: // ADD A,IXH
    state->a += (state->ix >> 8) & 0xFF;
    break;

  case 0x85: // ADD A,IXL
    state->a += state->ix & 0xFF;
    break;

  case 0x86: // ADD A,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->a += mem_read(temp16);
    break;

  case 0x8C: // ADC A,IXH
    state->a += (state->ix >> 8) & 0xFF + TST_FLAG(state, FLAG_C);
    break;

  case 0x8D: // ADC A,IXL
    state->a += state->ix & 0xFF + TST_FLAG(state, FLAG_C);
    break;

  case 0x8E: // ADC A,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->a += mem_read(temp16) + TST_FLAG(state, FLAG_C);
    break;

  case 0x94: // SUB A,IXH
    state->a -= (state->ix >> 8) & 0xFF;
    break;

  case 0x95: // SUB A,IXL
    state->a -= state->ix & 0xFF;
    break;

  case 0x96: // SUB A,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->a -= mem_read(temp16);
    break;

  case 0x9C: // SBC A,IXH
    state->a -= (state->ix >> 8) & 0xFF + TST_FLAG(state, FLAG_C);
    break;

  case 0x9D: // SBC A,IXL
    state->a -= state->ix & 0xFF + TST_FLAG(state, FLAG_C);
    break;

  case 0x9E: // SBC A,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->a -= mem_read(temp16) + TST_FLAG(state, FLAG_C);
    break;

  case 0xA4: // AND A,IXH
    state->a &= (state->ix >> 8) & 0xFF;
    UPDATE_SZ(state, state->a);
    break;

  case 0xA5: // AND A,IXL
    state->a &= state->ix & 0xFF;
    UPDATE_SZ(state, state->a);
    break;

  case 0xA6: // AND A,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->a &= mem_read(temp16);
    UPDATE_SZ(state, state->a);
    break;

  case 0xAC: // XOR A,IXH
    state->a ^= (state->ix >> 8) & 0xFF;
    UPDATE_SZ(state, state->a);
    break;

  case 0xAD: // XOR A,IXL
    state->a ^= state->ix & 0xFF;
    UPDATE_SZ(state, state->a);
    break;

  case 0xAE: // XOR A,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->a ^= mem_read(temp16);
    UPDATE_SZ(state, state->a);
    break;

  case 0xB4: // OR A,IXH
    state->a |= (state->ix >> 8) & 0xFF;
    UPDATE_SZ(state, state->a);
    break;

  case 0xB5: // OR A,IXL
    state->a |= state->ix & 0xFF;
    UPDATE_SZ(state, state->a);
    break;

  case 0xB6: // OR A,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    state->a |= mem_read(temp16);
    UPDATE_SZ(state, state->a);
    break;

  case 0xBC: // CP IXH
    temp = (state->ix >> 8) & 0xFF;
    res = state->a - temp;
    UPDATE_FLAGS_SUB(state, res, temp);
    break;

  case 0xBD: // CP IXL
    temp = state->ix & 0xFF;
    res = state->a - temp;
    UPDATE_FLAGS_SUB(state, res, temp);
    break;

  case 0xBE: // CP (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = state->a - temp;
    UPDATE_FLAGS_SUB(state, res, temp);
    break;

  case 0xCB: // CB-prefixed opcodes
    return decode_ddcb(state);

  case 0xE1: // POP IX
    state->ix = pop16(state);
    break;

  case 0xE3: // EX (SP),IX
    temp16 = pop16(state);
    push16(state, state->ix);
    state->ix = temp16;
    break;

  case 0xE5: // PUSH IX
    push16(state, state->ix);
    break;

  case 0xE9: // JP IX
    state->pc = state->ix;
    break;

  case 0xF9: // LD SP,IX
    state->sp = state->ix;
    break;

  default:
    printf("Unknown DD opcode: %02X\n", opcode);
    return 0;
  }
}

int decode_ddcb(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);
  uint8_t temp;
  uint16_t temp16;
  uint8_t n;
  uint8_t carry;
  uint8_t res;
  uint8_t port;

  switch (opcode) {

  case 0x00: // LD B,RLC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 7) | (temp << 1);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->b = res;
    break;

  case 0x01: // LD C,RLC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 7) | (temp << 1);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->c = res;
    break;

  case 0x02: // LD D,RLC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 7) | (temp << 1);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->d = res;
    break;

  case 0x03: // LD E,RLC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 7) | (temp << 1);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->e = res;
    break;

  case 0x04: // LD H,RLC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 7) | (temp << 1);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->h = res;
    break;

  case 0x05: // LD L,RLC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 7) | (temp << 1);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->l = res;
    break;

  case 0x06: // RLC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 7) | (temp << 1);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    mem_write(temp16, res);
    break;

  case 0x07: // LD A,RLC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 7) | (temp << 1);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->a = res;
    break;

  case 0x08: // LD B,RRC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->b = res;
    break;

  case 0x09: // LD C,RRC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->c = res;
    break;

  case 0x0a: // LD D,RRC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->d = res;
    break;

  case 0x0b: // LD E,RRC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->e = res;
    break;

  case 0x0c: // LD H,RRC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->h = res;
    break;

  case 0x0d: // LD L,RRC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->l = res;
    break;

  case 0x0e: // RRC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    mem_write(temp16, res);
    break;

  case 0x0f: // LD A,RRC (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->a = res;
    break;

  case 0x10: // LD B,RL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->b = res;
    break;

  case 0x11: // LD C,RL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->c = res;
    break;

  case 0x12: // LD D,RL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp << 1) | (temp >> 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->d = res;
    break;

  case 0x13: // LD E,RL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp << 1) | (temp >> 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->e = res;
    break;

  case 0x14: // LD H,RL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp << 1) | (temp >> 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->h = res;
    break;

  case 0x15: // LD L,RL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp << 1) | (temp >> 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->l = res;
    break;

  case 0x16: // RL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp << 1) | (temp >> 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    mem_write(temp16, res);
    break;

  case 0x17: // LD A,RL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp << 1) | (temp >> 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->a = res;
    break;

  case 0x18: // LD B,RR (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->b = res;
    break;

  case 0x19: // LD C,RR (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->c = res;
    break;

  case 0x1a: // LD D,RR (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->d = res;
    break;

  case 0x1b: // LD E,RR (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->e = res;
    break;

  case 0x1c: // LD H,RR (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->h = res;
    break;

  case 0x1d: // LD L,RR (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->l = res;
    break;

  case 0x1e: // RR (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    mem_write(temp16, res);
    break;

  case 0x1f: // LD A,RR (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = (temp >> 1) | (temp << 7);
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->a = res;
    break;

  case 0x20: // LD B,SLA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->b = res;
    break;

  case 0x21: // LD C,SLA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->c = res;
    break;

  case 0x22: // LD D,SLA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->d = res;
    break;

  case 0x23: // LD E,SLA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->e = res;
    break;

  case 0x24: // LD H,SLA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->h = res;
    break;

  case 0x25: // LD L,SLA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->l = res;
    break;

  case 0x26: // SLA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    mem_write(temp16, res);
    break;

  case 0x27: // LD A,SLA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->a = res;
    break;

  case 0x28: // LD B,SRA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->b = res;
    break;

  case 0x29: // LD C,SRA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->c = res;
    break;

  case 0x2a: // LD D,SRA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->d = res;
    break;

  case 0x2b: // LD E,SRA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->e = res;
    break;

  case 0x2c: // LD H,SRA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->h = res;
    break;

  case 0x2d: // LD L,SRA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->l = res;
    break;

  case 0x2e: // SRA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    mem_write(temp16, res);
    break;

  case 0x2f: // LD A,SRA (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->a = res;
    break;

  case 0x30: // LD B,SLL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->b = res;
    break;

  case 0x31: // LD C,SLL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->c = res;
    break;

  case 0x32: // LD D,SLL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->d = res;
    break;

  case 0x33: // LD E,SLL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->e = res;
    break;

  case 0x34: // LD H,SLL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->h = res;
    break;

  case 0x35: // LD L,SLL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->l = res;
    break;

  case 0x36: // SLL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    mem_write(temp16, res);
    break;

  case 0x37: // LD A,SLL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp << 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->a = res;
    break;

  case 0x38: // LD B,SRL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->b = res;
    break;

  case 0x39: // LD C,SRL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->c = res;
    break;

  case 0x3a: // LD D,SRL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->d = res;
    break;

  case 0x3b: // LD E,SRL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->e = res;
    break;

  case 0x3c: // LD H,SRL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->h = res;
    break;

  case 0x3d: // LD L,SRL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->l = res;
    break;

  case 0x3e: // SRL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    mem_write(temp16, res);
    break;

  case 0x3f: // LD A,SRL (IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp >> 1;
    state->f &= ~(FLAG_C | FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (res & FLAG_C) | (res ? 0 : FLAG_Z);
    state->a = res;
    break;

  case 0x47: // BIT 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    state->f &= ~(FLAG_N | FLAG_H | FLAG_S | FLAG_Z | FLAG_PV);
    state->f |= (temp & FLAG_S) | (temp & FLAG_PV);
    if(temp & 0x01)
      CLR_FLAG(state, FLAG_Z);
    else
      SET_FLAG(state, FLAG_Z);
    break;

  case 0x4f: // BIT 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    state->f &= ~(FLAG_N | FLAG_H | FLAG_S | FLAG_Z | FLAG_PV);
    state->f |= (temp & FLAG_S) | (temp & FLAG_PV);
    if(temp & 0x02)
      CLR_FLAG(state, FLAG_Z);
    else
      SET_FLAG(state, FLAG_Z);
    break;

  case 0x57: // BIT 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    state->f &= ~(FLAG_N | FLAG_H | FLAG_S | FLAG_Z | FLAG_PV);
    state->f |= (temp & FLAG_S) | (temp & FLAG_PV);
    if(temp & 0x04)
      CLR_FLAG(state, FLAG_Z);
    else
      SET_FLAG(state, FLAG_Z);
    break;

  case 0x5f: // BIT 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    state->f &= ~(FLAG_N | FLAG_H | FLAG_S | FLAG_Z | FLAG_PV);
    state->f |= (temp & FLAG_S) | (temp & FLAG_PV);
    if(temp & 0x08)
      CLR_FLAG(state, FLAG_Z);
    else
      SET_FLAG(state, FLAG_Z);
    break;

  case 0x67: // BIT 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    state->f &= ~(FLAG_N | FLAG_H | FLAG_S | FLAG_Z | FLAG_PV);
    state->f |= (temp & FLAG_S) | (temp & FLAG_PV);
    if(temp & 0x10)
      CLR_FLAG(state, FLAG_Z);
    else
      SET_FLAG(state, FLAG_Z);
    break;

  case 0x6f: // BIT 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    state->f &= ~(FLAG_N | FLAG_H | FLAG_S | FLAG_Z | FLAG_PV);
    state->f |= (temp & FLAG_S) | (temp & FLAG_PV);
    if(temp & 0x20)
      CLR_FLAG(state, FLAG_Z);
    else
      SET_FLAG(state, FLAG_Z);
    break;

  case 0x77: // BIT 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    state->f &= ~(FLAG_N | FLAG_H | FLAG_S | FLAG_Z | FLAG_PV);
    state->f |= (temp & FLAG_S) | (temp & FLAG_PV);
    if(temp & 0x40)
      CLR_FLAG(state, FLAG_Z);
    else
      SET_FLAG(state, FLAG_Z);
    break;

  case 0x7f: // BIT 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    state->f &= ~(FLAG_N | FLAG_H | FLAG_S | FLAG_Z | FLAG_PV);
    state->f |= (temp & FLAG_S) | (temp & FLAG_PV);
    if(temp & 0x80)
      CLR_FLAG(state, FLAG_Z);
    else
      SET_FLAG(state, FLAG_Z);
    break;

  case 0x80: // LD B,RES 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 0);
    state->b = res;
    break;

  case 0x81: // LD C,RES 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 0);
    state->c = res;
    break;

  case 0x82: // LD D,RES 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 0);
    state->d = res;
    break;

  case 0x83: // LD E,RES 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 0);
    state->e = res;
    break;

  case 0x84: // LD H,RES 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 0);
    state->h = res;
    break;

  case 0x85: // LD L,RES 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 0);
    state->l = res;
    break;

  case 0x86: // RES 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 0);
    mem_write(temp16, res);
    break;

  case 0x87: // LD A,RES 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 0);
    state->a = res;
    break;

  case 0x88: // LD B,RES 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 1);
    state->b = res;
    break;

  case 0x89: // LD C,RES 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 1);
    state->c = res;
    break;

  case 0x8a: // LD D,RES 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 1);
    state->d = res;
    break;

  case 0x8b: // LD E,RES 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 1);
    state->e = res;
    break;

  case 0x8c: // LD H,RES 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 1);
    state->h = res;
    break;

  case 0x8d: // LD L,RES 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 1);
    state->l = res;
    break;

  case 0x8e: // RES 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 1);
    mem_write(temp16, res);
    break;

  case 0x8f: // LD A,RES 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 1);
    state->a = res;
    break;

  case 0x90: // LD B,RES 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 2);
    state->b = res;
    break;

  case 0x91: // LD C,RES 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 2);
    state->c = res;
    break;

  case 0x92: // LD D,RES 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 2);
    state->d = res;
    break;

  case 0x93: // LD E,RES 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 2);
    state->e = res;
    break;

  case 0x94: // LD H,RES 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 2);
    state->h = res;
    break;

  case 0x95: // LD L,RES 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 2);
    state->l = res;
    break;

  case 0x96: // RES 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 2);
    mem_write(temp16, res);
    break;

  case 0x97: // LD A,RES 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 2);
    state->a = res;
    break;

  case 0x98: // LD B,RES 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 3);
    state->b = res;
    break;

  case 0x99: // LD C,RES 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 3);
    state->c = res;
    break;

  case 0x9a: // LD D,RES 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 3);
    state->d = res;
    break;

  case 0x9b: // LD E,RES 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 3);
    state->e = res;
    break;

  case 0x9c: // LD H,RES 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 3);
    state->h = res;
    break;

  case 0x9d: // LD L,RES 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 3);
    state->l = res;
    break;

  case 0x9e: // RES 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 3);
    mem_write(temp16, res);
    break;

  case 0x9f: // LD A,RES 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 3);
    state->a = res;
    break;

  case 0xa0: // LD B,RES 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 4);
    state->b = res;
    break;

  case 0xa1: // LD C,RES 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 4);
    state->c = res;
    break;

  case 0xa2: // LD D,RES 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 4);
    state->d = res;
    break;

  case 0xa3: // LD E,RES 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 4);
    state->e = res;
    break;

  case 0xa4: // LD H,RES 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 4);
    state->h = res;
    break;

  case 0xa5: // LD L,RES 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 4);
    state->l = res;
    break;

  case 0xa6: // RES 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 4);
    mem_write(temp16, res);
    break;

  case 0xa7: // LD A,RES 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 4);
    state->a = res;
    break;

  case 0xa8: // LD B,RES 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 5);
    state->b = res;
    break;

  case 0xa9: // LD C,RES 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 5);
    state->c = res;
    break;

  case 0xaa: // LD D,RES 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 5);
    state->d = res;
    break;

  case 0xab: // LD E,RES 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 5);
    state->e = res;
    break;

  case 0xac: // LD H,RES 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 5);
    state->h = res;
    break;

  case 0xad: // LD L,RES 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 5);
    state->l = res;
    break;

  case 0xae: // RES 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 5);
    mem_write(temp16, res);
    break;

  case 0xaf: // LD A,RES 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 5);
    state->a = res;
    break;

  case 0xb0: // LD B,RES 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 6);
    state->b = res;
    break;

  case 0xb1: // LD C,RES 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 6);
    state->c = res;
    break;

  case 0xb2: // LD D,RES 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 6);
    state->d = res;
    break;

  case 0xb3: // LD E,RES 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 6);
    state->e = res;
    break;

  case 0xb4: // LD H,RES 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 6);
    state->h = res;
    break;

  case 0xb5: // LD L,RES 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 6);
    state->l = res;
    break;

  case 0xb6: // RES 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 6);
    mem_write(temp16, res);
    break;

  case 0xb7: // LD A,RES 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 6);
    state->a = res;
    break;

  case 0xb8: // LD B,RES 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 7);
    state->b = res;
    break;

  case 0xb9: // LD C,RES 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 7);
    state->c = res;
    break;

  case 0xBA: // LD D,RES 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 7);
    state->d = res;
    break;

  case 0xBB: // LD E,RES 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 7);
    state->e = res;
    break;

  case 0xBC: // LD H,RES 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 7);
    state->h = res;
    break;

  case 0xBD: // LD L,RES 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 7);
    state->l = res;
    break;

  case 0xBE: // RES 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 7);
    mem_write(temp16, res);
    break;

  case 0xBF: // LD A,RES 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp & ~(1 << 7);
    state->a = res;
    break;

  case 0xC0: // LD B,SET 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 0);
    state->b = res;
    break;

  case 0xC1: // LD C,SET 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 0);
    state->c = res;
    break;

  case 0xC2: // LD D,SET 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 0);
    state->d = res;
    break;

  case 0xC3: // LD E,SET 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 0);
    state->e = res;
    break;

  case 0xC4: // LD H,SET 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 0);
    state->h = res;
    break;

  case 0xC5: // LD L,SET 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 0);
    state->l = res;
    break;

  case 0xC6: // SET 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 0);
    mem_write(temp16, res);
    break;

  case 0xC7: // LD A,SET 0,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 0);
    state->a = res;
    break;

  case 0xC8: // LD B,SET 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 1);
    state->b = res;
    break;

  case 0xC9: // LD C,SET 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 1);
    state->c = res;
    break;

  case 0xCA: // LD D,SET 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 1);
    state->d = res;
    break;

  case 0xCB: // LD E,SET 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 1);
    state->e = res;
    break;

  case 0xCC: // LD H,SET 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 1);
    state->h = res;
    break;

  case 0xCD: // LD L,SET 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 1);
    state->l = res;
    break;

  case 0xCE: // SET 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 1);
    mem_write(temp16, res);
    break;

  case 0xCF: // LD A,SET 1,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 1);
    state->a = res;
    break;

  case 0xD0: // LD B,SET 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 2);
    state->b = res;
    break;

  case 0xD1: // LD C,SET 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 2);
    state->c = res;
    break;

  case 0xD2: // LD D,SET 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 2);
    state->d = res;
    break;

  case 0xD3: // LD E,SET 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 2);
    state->e = res;
    break;

  case 0xD4: // LD H,SET 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 2);
    state->h = res;
    break;

  case 0xD5: // LD L,SET 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 2);
    state->l = res;
    break;

  case 0xD6: // SET 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 2);
    mem_write(temp16, res);
    break;

  case 0xD7: // LD A,SET 2,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 2);
    state->a = res;
    break;

  case 0xD8: // LD B,SET 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 3);
    state->b = res;
    break;

  case 0xD9: // LD C,SET 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 3);
    state->c = res;
    break;

  case 0xDA: // LD D,SET 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 3);
    state->d = res;
    break;

  case 0xDB: // LD E,SET 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 3);
    state->e = res;
    break;

  case 0xDC: // LD H,SET 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 3);
    state->h = res;
    break;

  case 0xDD: // LD L,SET 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 3);
    state->l = res;
    break;

  case 0xDE: // SET 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 3);
    mem_write(temp16, res);
    break;

  case 0xDF: // LD A,SET 3,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 3);
    state->a = res;
    break;

  case 0xE0: // LD B,SET 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 4);
    state->b = res;
    break;

  case 0xE1: // LD C,SET 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 4);
    state->c = res;
    break;

  case 0xE2: // LD D,SET 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 4);
    state->d = res;
    break;

  case 0xE3: // LD E,SET 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 4);
    state->e = res;
    break;

  case 0xE4: // LD H,SET 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 4);
    state->h = res;
    break;

  case 0xE5: // LD L,SET 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 4);
    state->l = res;
    break;

  case 0xE6: // SET 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 4);
    mem_write(temp16, res);
    break;

  case 0xE7: // LD A,SET 4,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 4);
    state->a = res;
    break;

  case 0xE8: // LD B,SET 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 5);
    state->b = res;
    break;

  case 0xE9: // LD C,SET 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 5);
    state->c = res;
    break;

  case 0xEA: // LD D,SET 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 5);
    state->d = res;
    break;

  case 0xEB: // LD E,SET 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 5);
    state->e = res;
    break;

  case 0xEC: // LD H,SET 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 5);
    state->h = res;
    break;

  case 0xED: // LD L,SET 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 5);
    state->l = res;
    break;

  case 0xEE: // SET 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 5);
    mem_write(temp16, res);
    break;

  case 0xEF: // LD A,SET 5,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 5);
    state->a = res;
    break;

  case 0xF0: // LD B,SET 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 6);
    state->b = res;
    break;

  case 0xF1: // LD C,SET 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 6);
    state->c = res;
    break;

  case 0xF2: // LD D,SET 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 6);
    state->d = res;
    break;

  case 0xF3: // LD E,SET 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 6);
    state->e = res;
    break;

  case 0xF4: // LD H,SET 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 6);
    state->h = res;
    break;

  case 0xF5: // LD L,SET 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 6);
    state->l = res;
    break;

  case 0xF6: // SET 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 6);
    mem_write(temp16, res);
    break;

  case 0xF7: // LD A,SET 6,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 6);
    state->a = res;
    break;

  case 0xF8: // LD B,SET 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 7);
    state->b = res;
    break;

  case 0xF9: // LD C,SET 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 7);
    state->c = res;
    break;

  case 0xFA: // LD D,SET 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 7);
    state->d = res;
    break;

  case 0xFB: // LD E,SET 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 7);
    state->e = res;
    break;

  case 0xFC: // LD H,SET 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 7);
    state->h = res;
    break;

  case 0xFD: // LD L,SET 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 7);
    state->l = res;
    break;

  case 0xFE: // SET 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 7);
    mem_write(temp16, res);
    break;

  case 0xFF: // LD A,SET 7,(IX+dd)
    temp16 = state->ix + (int8_t)mem_read(state->pc++);
    temp = mem_read(temp16);
    res = temp | (1 << 7);
    state->a = res;
    break;

  default:
    printf("Unknown DD CB opcode: %02X\n", opcode);
    return 0;
  }
}

int decode_ed(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);
  uint8_t temp;
  uint16_t temp16;
  uint8_t n;
  uint8_t carry;
  uint8_t res;
  uint8_t port;

  switch (opcode) {
  default:
    printf("Unknown ED opcode: %02X\n", opcode);
    return 0;
  }
}

int decode_fd(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);
  uint8_t temp;
  uint16_t temp16;
  uint8_t n;
  uint8_t carry;
  uint8_t res;
  uint8_t port;

  switch (opcode) {
  case 0xCB:
    return decode_fdcb(state);
  default:
    printf("Unknown FD opcode: %02X\n", opcode);
    return 0;
  }
}

int decode_fdcb(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);
  uint8_t temp;
  uint16_t temp16;
  uint8_t n;
  uint8_t carry;
  uint8_t res;
  uint8_t port;

  switch (opcode) {
  default:
    printf("Unknown FD CB opcode: %02X\n", opcode);
    return 0;
  }
}

int z80_step(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);
  uint8_t temp;
  uint16_t temp16;
  uint8_t n;
  uint8_t carry;
  uint8_t res;
  uint8_t port;
  uint8_t tempA;
  uint8_t tempF;

  switch (opcode) {

  case 0x00: // NOP
    break;

  case 0x01: // LD BC,nn
    state->bc = (mem_read(state->pc++) << 8) | mem_read(state->pc++);
    break;

  case 0x02: // LD (BC),A
    mem_write(state->bc, state->a);
    break;

  case 0x03: // INC BC
    state->bc++;
    break;

  case 0x04: // INC B
    state->b++;
    UPDATE_SZ(state, state->b);
    break;

  case 0x05: // DEC B
    state->b--;
    UPDATE_SZ(state, state->b);
    SET_FLAG(state, FLAG_N);
    break;

  case 0x06: // LD B,n
    state->b = mem_read(state->pc++);
    break;

  case 0x07: // RLCA
    state->a = (state->a << 1) | (state->a >> 7);
    UPDATE_SZ(state, state->a);
    break;

  case 0x08: // EX AF, AF'
    tempA = state->a;
    tempF = state->f;
    state->a = state->a_;
    state->f = state->f_;
    state->a_ = tempA;
    state->f_ = tempF;
    break;

  case 0x09: // ADD HL,BC
    temp16 = state->hl + state->bc;
    CLR_FLAG(state, FLAG_N | FLAG_H | FLAG_C);
    if (((state->hl & 0x0FFF) + (state->bc & 0x0FFF)) > 0x0FFF) SET_FLAG(state, FLAG_H);
    if (temp16 < state->hl) SET_FLAG(state, FLAG_C);
    state->hl = temp16;
    break;

  case 0x0A: // LD A,(BC)
    state->a = mem_read(state->bc);
    break;

  case 0x0B: // DEC BC
    state->bc--;
    break;

  case 0x0C: // INC C
    state->c++;
    UPDATE_SZ(state, state->c);
    break;

  case 0x0D: // DEC C
    state->c--;
    UPDATE_SZ(state, state->c);
    SET_FLAG(state, FLAG_N);
    break;

  case 0x0E: // LD C,n
    state->c = mem_read(state->pc++);
    break;

  case 0x0F: // RRCA
    state->a = (state->a >> 1) | (state->a << 7);
    UPDATE_SZ(state, state->a);
    break;

  case 0x10: // DJNZ n
    temp = mem_read(state->pc + 1);
    state->b--;
    if (state->b != 0) {
      state->pc += temp;
    }
    break;

  case 0x11: // LD DE,nn
    state->de = (mem_read(state->pc++) << 8) | mem_read(state->pc++);
    break;

  case 0x12: // LD (DE),A
    mem_write(state->de, state->a);
    break;

  case 0x13: // INC DE
    state->de++;
    break;

  case 0x14: // INC D
    state->d++;
    UPDATE_SZ(state, state->d);
    break;

  case 0x15: // DEC D
    state->d--;
    UPDATE_SZ(state, state->d);
    SET_FLAG(state, FLAG_N);
    break;

  case 0x16: // LD D,n
    state->d = mem_read(state->pc++);
    break;

  case 0x17: // RLA
    temp = (state->a >> 7) & 1;
    state->a = (state->a << 1) | (state->f & FLAG_C);
    UPDATE_SZ(state, state->a);
    state->f = (state->f & ~FLAG_C) | (temp << 4);
    break;

  case 0x18: // JR n
    temp = mem_read(state->pc + 1);
    state->pc += temp;
    break;

  case 0x19: // ADD HL,DE
    temp16 = state->hl + state->de;
    CLR_FLAG(state, FLAG_N | FLAG_H | FLAG_C);
    if (((state->hl & 0x0FFF) + (state->de & 0x0FFF)) > 0x0FFF) SET_FLAG(state, FLAG_H);
    if (temp16 < state->hl) SET_FLAG(state, FLAG_C);
    state->hl = temp16;
    break;

  case 0x1A: // LD A,(DE)
    state->a = mem_read(state->de);
    break;

  case 0x1B: // DEC DE
    state->de--;
    break;

  case 0x1C: // INC E
    state->e++;
    UPDATE_SZ(state, state->e);
    break;

  case 0x1D: // DEC E
    state->e--;
    UPDATE_SZ(state, state->e);
    SET_FLAG(state, FLAG_N);
    break;

  case 0x1E: // LD E,n
    state->e = mem_read(state->pc++);
    break;

  case 0x1F: // RRA
    temp = (state->a & 1);
    state->a = (state->a >> 1) | ((state->f & FLAG_C) << 7);
    UPDATE_SZ(state, state->a);
    state->f = (state->f & ~FLAG_C) | (temp << 4);
    break;

  case 0x20: // JR NZ, n
    temp = mem_read(state->pc + 1);
    if ((state->f & FLAG_Z) == 0) {
      state->pc += temp;
    }
    break;

  case 0x21: // LD HL,nn
    state->hl = (mem_read(state->pc++) << 8) | mem_read(state->pc++);
    break;

  case 0x22: // LD (nn),HL
    mem_write((mem_read(state->pc++) << 8) | mem_read(state->pc++), state->l);
    mem_write((mem_read(state->pc++) << 8) | mem_read(state->pc++), state->h);
    break;

  case 0x23: // INC HL
    state->hl++;
    break;

  case 0x24: // INC H
    state->h++;
    UPDATE_SZ(state, state->h);
    break;

  case 0x25: // DEC H
    state->h--;
    UPDATE_SZ(state, state->h);
    SET_FLAG(state, FLAG_N);
    break;

  case 0x26: // LD H,n
    state->h = mem_read(state->pc++);
    break;

  case 0x27: // DAA
    temp = state->a;
    if ((state->f & FLAG_C) || (state->a > 0x99)) {
      state->a += 0x60;
      SET_FLAG(state, FLAG_C);
    }
    if ((state->f & FLAG_H) || (state->a & 0x0F) > 0x09) {
      state->a += 0x06;
      SET_FLAG(state, FLAG_H);
    }
    UPDATE_SZ(state, state->a);
    break;

  case 0x28: // JR Z, n
    temp = mem_read(state->pc + 1);
    if ((state->f & FLAG_Z) != 0) {
      state->pc += temp;
    }
    break;

  case 0x29: // ADD HL,HL
    temp16 = state->hl + state->hl;
    CLR_FLAG(state, FLAG_N | FLAG_H | FLAG_C);
    if (((state->hl & 0x0FFF) + (state->hl & 0x0FFF)) > 0x0FFF) SET_FLAG(state, FLAG_H);
    if (temp16 < state->hl) SET_FLAG(state, FLAG_C);
    state->hl = temp16;
    break;

  case 0x2A: // LD HL,(nn)
    temp16 = mem_read(state->pc++);
    temp16 |= mem_read(state->pc++) << 8;
    state->l = mem_read(temp16);
    state->h = mem_read(temp16 + 1);
    break;

  case 0x2B: // LD HL,nn
    state->l = mem_read(state->pc++);
    state->h = mem_read(state->pc++);
    break;

  case 0x2C: // LD (HL),A
    mem_write(state->hl, state->a);
    break;

  case 0x2D: // DEC HL
    state->hl--;
    break;

  case 0x2E: // LD L,n
    state->l = mem_read(state->pc++);
    break;

  case 0x2F: // CPL
    state->a = ~state->a;
    SET_FLAG(state, FLAG_N | FLAG_H);
    break;

  case 0x30: // JR NC, n
    temp = mem_read(state->pc + 1);
    if ((state->f & FLAG_C) == 0) {
      state->pc += temp;
    }
    break;

  case 0x31: // LD SP,nn
    state->sp = (mem_read(state->pc++) << 8) | mem_read(state->pc++);
    break;

  case 0x32: // LD (nn),A
    mem_write((mem_read(state->pc++) << 8) | mem_read(state->pc++), state->a);
    break;

  case 0x33: // INC SP
    state->sp++;
    break;

  case 0x34: // INC (HL)
    temp = mem_read(state->hl);
    temp++;
    mem_write(state->hl, temp);
    UPDATE_SZ(state, temp);
    break;

  case 0x35: // DEC (HL)
    temp = mem_read(state->hl);
    temp--;
    mem_write(state->hl, temp);
    UPDATE_SZ(state, temp);
    SET_FLAG(state, FLAG_N);
    break;

  case 0x36: // LD (HL),n
    mem_write(state->hl, mem_read(state->pc++));
    break;

  case 0x37: // SCF
    CLR_FLAG(state, FLAG_N | FLAG_H);
    SET_FLAG(state, FLAG_C);
    break;

  case 0x38: // JR C, n
    temp = mem_read(state->pc + 1);
    if ((state->f & FLAG_C) != 0) {
      state->pc += temp;
    }
    break;

  case 0x39: // ADD HL,SP
    temp = state->sp + state->hl;
    CLR_FLAG(state, FLAG_N);
    if (temp & 0x10000) SET_FLAG(state, FLAG_C);
    if (((state->hl ^ state->sp ^ temp) & 0x1000) == 0x1000) SET_FLAG(state, FLAG_H);
    state->hl = temp & 0xFFFF;
    break;

  case 0x3A: // LD A,(nn)
    temp16 = mem_read(state->pc++);
    temp16 |= mem_read(state->pc++) << 8;
    state->a = mem_read(temp16);
    break;

  case 0x3B: // DEC SP
    state->sp--;
    break;

  case 0x3C: // INC A
    state->a++;
    UPDATE_SZ(state, state->a);
    break;

  case 0x3D: // DEC A
    state->a--;
    UPDATE_SZ(state, state->a);
    SET_FLAG(state, FLAG_N);
    break;

  case 0x3E: // LD A,n
    state->a = mem_read(state->pc++);
    break;

  case 0x3F: // CCF
    state->f ^= FLAG_C; // Toggle carry flag
    CLR_FLAG(state, FLAG_N | FLAG_H); // Clear N and H flags
    if (state->f & FLAG_C) SET_FLAG(state, FLAG_H); // Set H if carry is set
    break;

  case 0x40: // LD B,B
    state->b = state->b;
    break;

  case 0x41: // LD B,C
    state->b = state->c;
    break;

  case 0x42: // LD B,D
    state->b = state->d;
    break;

  case 0x43: // LD B,E
    state->b = state->e;
    break;

  case 0x44: // LD B,H
    state->b = state->h;
    break;

  case 0x45: // LD B,L
    state->b = state->l;
    break;

  case 0x46: // LD B,(HL)
    state->b = mem_read(state->hl);
    break;

  case 0x47: // LD B,A
    state->b = state->a;
    break;

  case 0x48: // LD C,B
    state->c = state->b;
    break;

  case 0x49: // LD C,C
    state->c = state->c;
    break;

  case 0x4A: // LD C,D
    state->c = state->d;
    break;

  case 0x4B: // LD C,E
    state->c = state->e;
    break;

  case 0x4C: // LD C,H
    state->c = state->h;
    break;

  case 0x4D: // LD C,L
    state->c = state->l;
    break;

  case 0x4E: // LD C,(HL)
    state->c = mem_read(state->hl);
    break;

  case 0x4F: // LD C,A
    state->c = state->a;
    break;

  case 0x50: // LD D,B
    state->d = state->b;
    break;

  case 0x51: // LD D,C
    state->d = state->c;
    break;

  case 0x52: // LD D,D
    state->d = state->d;
    break;

  case 0x53: // LD D,E
    state->d = state->e;
    break;

  case 0x54: // LD D,H
    state->d = state->h;
    break;

  case 0x55: // LD D,L
    state->d = state->l;
    break;

  case 0x56: // LD D,(HL)
    state->d = mem_read(state->hl);
    break;

  case 0x57: // LD D,A
    state->d = state->a;
    break;

  case 0x58: // LD E,B
    state->e = state->b;
    break;

  case 0x59: // LD E,C
    state->e = state->c;
    break;

  case 0x5A: // LD E,D
    state->e = state->d;
    break;

  case 0x5B: // LD E,E
    state->e = state->e;
    break;

  case 0x5C: // LD E,H
    state->e = state->h;
    break;

  case 0x5D: // LD E,L
    state->e = state->l;
    break;

  case 0x5E: // LD E,(HL)
    state->e = mem_read(state->hl);
    break;

  case 0x5F: // LD E,A
    state->e = state->a;
    break;

  case 0x60: // LD H,B
    state->h = state->b;
    break;

  case 0x61: // LD H,C
    state->h = state->c;
    break;

  case 0x62: // LD H,D
    state->h = state->d;
    break;

  case 0x63: // LD H,E
    state->h = state->e;
    break;

  case 0x64: // LD H,H
    state->h = state->h;
    break;

  case 0x65: // LD H,L
    state->h = state->l;
    break;

  case 0x66: // LD H,(HL)
    state->h = mem_read(state->hl);
    break;

  case 0x67: // LD H,A
    state->h = state->a;
    break;

  case 0x68: // LD L,B
    state->l = state->b;
    break;

  case 0x69: // LD L,C
    state->l = state->c;
    break;

  case 0x6A: // LD L,D
    state->l = state->d;
    break;

  case 0x6B: // LD L,E
    state->l = state->e;
    break;

  case 0x6C: // LD L,H
    state->l = state->h;
    break;

  case 0x6D: // LD L,L
    state->l = state->l;
    break;

  case 0x6E: // LD L,(HL)
    state->l = mem_read(state->hl);
    break;

  case 0x6F: // LD L,A
    state->l = state->a;
    break;

  case 0x70: // LD (HL),B
    mem_write(state->hl, state->b);
    break;

  case 0x71: // LD (HL),C
    mem_write(state->hl, state->c);
    break;

  case 0x72: // LD (HL),D
    mem_write(state->hl, state->d);
    break;

  case 0x73: // LD (HL),E
    mem_write(state->hl, state->e);
    break;

  case 0x74: // LD (HL),H
    mem_write(state->hl, state->h);
    break;

  case 0x75: // LD (HL),L
    mem_write(state->hl, state->l);
    break;

  case 0x76: // HALT
    // TODO: Implement HALT instruction
    break;

  case 0x77: // LD (HL),A
    mem_write(state->hl, state->a);
    break;

  case 0x78: // LD A,B
    state->a = state->b;
    break;

  case 0x79: // LD A,C
    state->a = state->c;
    break;

  case 0x7A: // LD A,D
    state->a = state->d;
    break;

  case 0x7B: // LD A,E
    state->a = state->e;
    break;

  case 0x7C: // LD A,H
    state->a = state->h;
    break;

  case 0x7D: // LD A,L
    state->a = state->l;
    break;

  case 0x7E: // LD A,(HL)
    state->a = mem_read(state->hl);
    break;

  case 0x7F: // LD A,A
    state->a = state->a;
    break;

  case 0x80: // ADD A, B
    temp = state->a + state->b;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->b & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->b ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x81: // ADD A, C
    temp = state->a + state->c;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->c & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->c ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x82: // ADD A, D
    temp = state->a + state->d;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->d & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->d ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x83: // ADD A, E
    temp = state->a + state->e;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->e & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->e ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x84: // ADD A, H
    temp = state->a + state->h;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->h & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->h ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x85: // ADD A, L
    temp = state->a + state->l;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->l & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->l ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x86: // ADD A, (HL)
    temp = state->a + mem_read(state->hl);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (mem_read(state->hl) & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ mem_read(state->hl) ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x87: // ADD A, A
    temp = state->a + state->a;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->a & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->a ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x88: // ADC A, B
    temp = state->a + state->b + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->b & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->b ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x89: // ADC A, C
    temp = state->a + state->c + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->c & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->c ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8A: // ADC A, D
    temp = state->a + state->d + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->d & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->d ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8B: // ADC A, E
    temp = state->a + state->e + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->e & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->e ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8C: // ADC A, H
    temp = state->a + state->h + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->h & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->h ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8D: // ADC A, L
    temp = state->a + state->l + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->l & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->l ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8E: // ADC A, (HL)
    temp = state->a + mem_read(state->hl) + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (mem_read(state->hl) & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ mem_read(state->hl) ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8F: // ADC A, A
    temp = state->a + state->a + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->a & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->a ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x90: // SUB B
    temp = state->a - state->b;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->b & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->b) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x91: // SUB C
    temp = state->a - state->c;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->c & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->c) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x92: // SUB D
    temp = state->a - state->d;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->d & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->d) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x93: // SUB E
    temp = state->a - state->e;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->e & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->e) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x94: // SUB H
    temp = state->a - state->h;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->h & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->h) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x95: // SUB L
    temp = state->a - state->l;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->l & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->l) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x96: // SUB (HL)
    temp = state->a - mem_read(state->hl);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (mem_read(state->hl) & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < mem_read(state->hl)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x97: // SUB A
    temp = state->a - state->a;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->a & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->a) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x98: // SBC A, B
    temp = state->a - state->b - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->b & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->b + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x99: // SBC A, C
    temp = state->a - state->c - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->c & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->c + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9A: // SBC A, D
    temp = state->a - state->d - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->d & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->d + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9B: // SBC A, E
    temp = state->a - state->e - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->e & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->e + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9C: // SBC A, H
    temp = state->a - state->h - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->h & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->h + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9D: // SBC A, L
    temp = state->a - state->l - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->l & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->l + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9E: // SBC A, (HL)
    temp = state->a - mem_read(state->hl) - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (mem_read(state->hl) & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < mem_read(state->hl) + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9F: // SBC A, A
    temp = state->a - state->a - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->a & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->a + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0xA0: // AND B
    state->a = state->a & state->b;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA1: // AND C
    state->a = state->a & state->c;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA2: // AND D
    state->a = state->a & state->d;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA3: // AND E
    state->a = state->a & state->e;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA4: // AND H
    state->a = state->a & state->h;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA5: // AND L
    state->a = state->a & state->l;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA6: // AND (HL)
    state->a = state->a & mem_read(state->hl);
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA7: // AND A
    state->a = state->a & state->a;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA8: // XOR B
    state->a = state->a ^ state->b;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA9: // XOR C
    state->a = state->a ^ state->c;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAA: // XOR D
    state->a = state->a ^ state->d;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAB: // XOR E
    state->a = state->a ^ state->e;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAC: // XOR H
    state->a = state->a ^ state->h;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAD: // XOR L
    state->a = state->a ^ state->l;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAE: // XOR (HL)
    state->a = state->a ^ mem_read(state->hl);
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAF: // XOR A
    state->a = state->a ^ state->a;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB0: // OR B
    state->a = state->a | state->b;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB1: // OR C
    state->a = state->a | state->c;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB2: // OR D
    state->a = state->a | state->d;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB3: // OR E
    state->a = state->a | state->e;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB4: // OR H
    state->a = state->a | state->h;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB5: // OR L
    state->a = state->a | state->l;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB6: // OR (HL)
    state->a = state->a | mem_read(state->hl);
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB7: // OR A
    state->a = state->a | state->a;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB8: // CP B
    temp = state->a - state->b;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->b & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->b) ? FLAG_N : 0;
    break;

  case 0xB9: // CP C
    temp = state->a - state->c;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->c & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->c) ? FLAG_N : 0;
    break;

  case 0xBA: // CP D
    temp = state->a - state->d;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->d & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->d) ? FLAG_N : 0;
    break;

  case 0xBB: // CP E
    temp = state->a - state->e;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->e & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->e) ? FLAG_N : 0;
    break;

  case 0xBC: // CP H
    temp = state->a - state->h;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->h & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->h) ? FLAG_N : 0;
    break;

  case 0xBD: // CP L
    temp = state->a - state->l;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->l & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->l) ? FLAG_N : 0;
    break;

  case 0xBE: // CP (HL)
    temp = state->a - mem_read(state->hl);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (mem_read(state->hl) & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < mem_read(state->hl)) ? FLAG_N : 0;
    break;

  case 0xBF: // CP A
    temp = state->a - state->a;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->a & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->a) ? FLAG_N : 0;
    break;

  case 0xC0:
    if ((state->f & FLAG_Z) == 0) {
      state->pc = mem_read16(state->sp);
      state->sp += 2;
    }
    else {
      state->pc += 2;
    }
    break;

  case 0xC1: // POP BC
    state->bc = pop16(state);
    break;

  case 0xC2: // JP NZ, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_Z) == 0) {
      state->pc = temp16;
    }
    break;

  case 0xC3: // JP nn
    state->pc = mem_read16(state->pc + 1);
    break;

  case 0xC4: // CALL NZ, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_Z) == 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xC5: // PUSH BC
    push16(state, state->bc);
    break;

  case 0xC6: // ADD A, n
    temp = state->a + mem_read(state->pc + 1);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (mem_read(state->pc + 1) & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < mem_read(state->pc + 1)) ? FLAG_N : 0;
    state->a = temp;
    state->pc += 2;
    break;

  case 0xC7: // RST 0
    push16(state, state->pc + 1);
    state->pc = 0x0000;
    break;

  case 0xC8: // RET Z
    if ((state->f & FLAG_Z) != 0) {
      state->pc = mem_read16(state->sp);
      state->sp += 2;
    }
    break;

  case 0xC9: // RET
    state->pc = mem_read16(state->sp);
    state->sp += 2;
    break;

  case 0xCA: // JP Z, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_Z) != 0) {
      state->pc = temp16;
    }
    break;

  case 0xCB: // CB prefixed instructions
    return decode_cb(state);
  case 0xCC: // CALL Z, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_Z) != 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xCD: // CALL nn
    temp16 = mem_read16(state->pc + 1);
    push16(state, state->pc + 3);
    state->pc = temp16;
    break;

  case 0xCE: // ADC A, n
    temp = state->a + mem_read(state->pc + 1);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (mem_read(state->pc + 1) & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < mem_read(state->pc + 1)) ? FLAG_N : 0;
    state->f |= (state->a & 0x0F) < (mem_read(state->pc + 1) & 0x0F) ? FLAG_C : 0;
    state->a = temp;
    state->pc += 2;
    break;

  case 0xCF: // RST 8
    push16(state, state->pc + 1);
    state->pc = 0x0038;
    break;

  case 0xD0: // RET NC
    if ((state->f & FLAG_C) == 0) {
      state->pc = mem_read16(state->sp);
      state->sp += 2;
    }
    break;

  case 0xD1: // POP DE
    state->de = pop16(state);
    break;

  case 0xD2: // JP NC, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) == 0) {
      state->pc = temp16;
    }
    break;

  case 0xD3: // OUT (n), A
    n = mem_read(state->pc + 1);
    output_port(state, n, state->a);
    state->pc += 2;
    break;

  case 0xD4: // CALL NC, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) == 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xD5: // PUSH DE
    push16(state, state->de);
    break;

  case 0xD6: // SUB n
    n = mem_read(state->pc + 1);
    temp = state->a - n;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (n & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->a = temp;
    break;

  case 0xD7: // RST 10
    push16(state, state->pc + 1);
    state->pc = 0x0010;
    break;

  case 0xD8: // RET C
    if ((state->f & FLAG_C) != 0) {
      state->pc = mem_read16(state->sp);
      state->sp += 2;
    }
    break;

  case 0xD9: // EXX
    temp16 = state->bc;
    state->bc = state->bc_;
    state->bc_ = temp16;
    temp16 = state->de;
    state->de = state->de_;
    state->de_ = temp16;
    temp16 = state->hl;
    state->hl = state->hl_;
    state->hl_ = temp16;
    state->pc += 1;
    break;

  case 0xDA: // JP C, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) != 0) {
      state->pc = temp16;
    }
    break;

  case 0xDB: // IN A, (n)
    n = mem_read(state->pc + 1);
    state->a = input_port(state, n);
    state->pc += 2;
    break;

  case 0xDC: // CALL C, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) != 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xDD: // DD prefix
    return decode_dd(state);
    break;

  case 0xDE: // SBC A, n
    n = mem_read(state->pc + 1);
    carry = (state->f & FLAG_C) != 0 ? 1 : 0;
    temp = state->a - n - carry;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (n & 0x0F) + carry ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->a = temp;
    break;

  case 0xDF: // RST 30H
    push16(state, state->pc + 1);
    state->pc = 0x30;
    break;

  case 0xE0: // RET PO
    if ((state->f & FLAG_PV) == 0) {
      state->pc = mem_read16(state->sp);
      state->sp += 2;
    }
    break;

  case 0xE1: // POP HL
    state->hl = mem_read16(state->sp);
    state->sp += 2;
    break;

  case 0xE2: // JP PO, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_PV) == 0) {
      state->pc = temp16;
    }
    break;

  case 0xE3: // EX (SP), HL
    temp16 = mem_read16(state->sp);
    mem_write16(state->sp, state->hl);
    state->hl = temp16;
    break;

  case 0xE4: // CALL PO, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_PV) == 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xE5: // PUSH HL
    push16(state, state->hl);
    break;

  case 0xE6: // AND n
    n = mem_read(state->pc + 1);
    state->a &= n;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f &= ~FLAG_PV;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    break;

  case 0xE7: // RST 20H
    push16(state, state->pc + 1);
    state->pc = 0x20;
    break;

  case 0xE8: // RET PE
    if ((state->f & FLAG_PV) != 0) {
      state->pc = mem_read16(state->sp);
      state->sp += 2;
    }
    break;

  case 0xE9: // JP PE, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_PV) != 0) {
      state->pc = temp16;
    }
    break;

  case 0xEA: // JP C, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) != 0) {
      state->pc = temp16;
    }
    break;

  case 0xEB: // XB EE
    printf("Unknown opcode: %02X\n", opcode);
    return 0;
    break;

  case 0xEC: // CALL C, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) != 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xED: // ED-prefixed opcodes
    return decode_ed(state);
  case 0xEE: // XOR n
    n = mem_read(state->pc + 1);
    state->a ^= n;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f &= ~FLAG_PV;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    break;

  case 0xEF: // RST 28H
    temp16 = state->pc;
    state->sp -= 2;
    mem_write(state->sp, (temp >> 8) & 0xFF);
    mem_write(state->sp + 1, temp & 0xFF);
    state->pc = 0x28;
    break;

  case 0xF0: // RET P
    if ((state->f & FLAG_S) == 0) {
      state->pc = mem_read16(state->sp);
      state->sp += 2;
    }
    break;

  case 0xF1: // POP AF
    state->af = mem_read16(state->sp);
    state->sp += 2;
    break;

  case 0xF2: // JP P, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_S) == 0) {
      state->pc = temp16;
    }
    break;

  case 0xF3: // DI
    state->iff1 = 0;
    state->iff2 = 0;
    break;

  case 0xF4: // CALL P, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_S) == 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xF5: // PUSH AF
    state->sp -= 2;
    mem_write(state->sp, (state->af >> 8) & 0xFF);
    mem_write(state->sp + 1, state->af & 0xFF);
    break;

  case 0xF6: // OR n
    n = mem_read(state->pc + 1);
    state->a |= n;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f &= ~FLAG_PV;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    break;

  case 0xF7: // RST 30H
    push16(state, state->pc + 1);
    state->pc = 0x30;
    break;

  case 0xF8: // RET M
    if ((state->f & FLAG_S) != 0) {
      state->pc = mem_read16(state->sp);
      state->sp += 2;
    }
    break;

  case 0xF9: // LD SP, HL
    state->sp = state->hl;
    break;

  case 0xFA: // JP M, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_S) != 0) {
      state->pc = temp16;
    }
    break;

  case 0xFB: // EI
    state->iff1 = state->iff2 = 1;
    break;

  case 0xFC: // CALL M, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_S) != 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xFD: // FD prefix
    return decode_fd(state);
  case 0xFE: // CP n
    n = mem_read(state->pc + 1);
    temp = state->a - n;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (n & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < n) ? FLAG_N : 0;
    break;

  case 0xFF: // RST 38H
    push16(state, state->pc + 1);
    state->pc = 0x38;
    break;

  default:
    fprintf(stderr, "Unimplemented opcode: 0x%02X at 0x%04X\n",
      opcode, state->pc - 1);
    return -1;
  }

  return 0;
}

void push16(Z80_State* state, uint16_t val) {
  mem_write(--state->sp, (val >> 8) & 0xFF);
  mem_write(--state->sp, val & 0xFF);
}

uint16_t pop16(Z80_State* state) {
  uint16_t lo = mem_read(state->sp++);
  uint16_t hi = mem_read(state->sp++);
  return (hi << 8) | lo;
}