#include "pfa.h"
#include "frontend.h"
#include <stdio.h>
#include <stdlib.h>
void pfa_init()
{
  // create virtual mapping for PFA I/O area
  __map_kernel_range(PFA_BASE, PFA_BASE, RISCV_PGSIZE, PROT_READ|PROT_WRITE|PROT_EXEC);

  /* Provide scratch page for PFA */
  void *pfa_scratch = (void*)page_alloc();
  uintptr_t paddr = va2pa(pfa_scratch);
 
  
  /*
  if(test_mac()) {
    printk("Test mac passed\n");
  } else {
    printk("XX Test mac failed");
  }*/

  /*
  if(test_send_one()) {
    printk("Test send one passed\n");
  } else {
    printk("XX Test send one failed\n");
  }
  */
  if(test_recv()) {
    printk("Test recv passed\n");
  } else {
    printk("XX Test recv failed\n");
  }
  return;
}

uint64_t pfa_check_freeframes(void) {
  return *PFA_FREESTAT;
}

void pfa_publish_freeframe(uintptr_t paddr)
{
  assert(*PFA_FREESTAT > 0);
  *PFA_FREEFRAME = paddr;
}

pgid_t pfa_evict_page(void const *page)
{
  static pgid_t pgid = 0;
  uintptr_t paddr = va2pa(page);

  /* pfn goes in first 36bits, pgid goes in upper 28
   * See pfa spec for details. */
  uint64_t evict_val = paddr >> RISCV_PGSHIFT;
  assert(evict_val >> 36 == 0);
  assert(pgid >> 28 == 0);
  evict_val |= (uint64_t)pgid << 36;
  *PFA_EVICTPAGE = evict_val;

  pte_t *page_pte = walk((uintptr_t) page);
  /* At the moment, the page_id is just the page-aligned vaddr */
  *page_pte = pfa_mk_remote_pte(pgid, *page_pte);
  flush_tlb();

  return pgid++;
}

bool pfa_poll_evict(void)
{
  int poll_count = 0;
  volatile uint64_t *evictstat = PFA_EVICTSTAT;
  while(*evictstat < PFA_EVICT_MAX) {
    if(poll_count++ == MAX_POLL_ITER) {
      printk("Polling for eviction completion took too long\n");
      return false;
    }
  }

  return true;
}

pgid_t pfa_pop_newpage()
{
  /*XXX Discard the vaddr for now */
  volatile uint64_t vaddr = *PFA_NEWVADDR;
  return (pgid_t)(*PFA_NEWPGID);
}

uint64_t pfa_check_newpage()
{
  return *PFA_NEWSTAT;
}

/* Drain the new page queue without checking return values */
void pfa_drain_newq(void)
{
  uint64_t nnew = pfa_check_newpage();
  while(nnew) {
    pfa_pop_newpage();
    nnew--;
  }
  return;
}

pte_t pfa_mk_remote_pte(uint64_t page_id, pte_t orig_pte)
{
  pte_t rem_pte;

  /* page_id needs must fit in upper bits of PTE */
  assert(page_id >> (64 - PFA_PAGEID_SHIFT) == 0);


  /* Page ID */
  rem_pte = page_id << PFA_PAGEID_SHIFT;
  /* Protection Bits */
  rem_pte |= (orig_pte & ~(-1 << PTE_PPN_SHIFT)) << PFA_PROT_SHIFT;
  /* Valid and Remote Flags */
  rem_pte |= PFA_REMOTE;

  return rem_pte;
}

inline bool pfa_is_newqueue_empty(void)
{
  return *PFA_NEWSTAT == 0;
}

inline bool pfa_is_evictqueue_empty(void)
{
  return *PFA_EVICTSTAT == PFA_EVICT_MAX;
}

inline bool pfa_is_freequeue_empty(void)
{
  return *PFA_FREESTAT == PFA_FREE_MAX;
}

static inline int send_req_avail()
{
  uint16_t counts = *ICENET_COUNTS;
  return (counts) & 0xf;
}

static inline int recv_req_avail()
{
  uint16_t counts = *ICENET_COUNTS;
  return (counts >> 4) & 0xf;
}

static inline int send_comp_avail()
{
  uint16_t counts = *ICENET_COUNTS;
  return (counts >> 8) & 0xf;
}

static inline int recv_comp_avail()
{
  uint16_t counts = *ICENET_COUNTS;
  return (counts >> 12) & 0xf;
}


bool test_mac() {
  uint64_t mac = *ICENET_MACADDR;
  printk("Attatched to Network Interface.\n"
     "\tmac address is: %ld\n",
       mac);
  return true;
}


bool test_send_one() {
  if(send_req_avail() != 15) {
    printk("send_req_avail() %d != 15\n", send_req_avail());
    return false;
  }
  unsigned char msg[1500] = {'a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e','a','b','c','d','e'};                      
  uintptr_t addr = va2pa((void *)msg);
  
  uint64_t packet;
  uint64_t len = 1500;
  packet = (len << 48) | (addr & 0xffffffffffffL);

  *ICENET_SEND_REQ = packet;
  if(send_comp_avail() != 1) {
    printk("send_comp_avail() %d != 1\n", send_comp_avail());
    return false;
  }
  
  *ICENET_SEND_REQ = packet;
  if(send_comp_avail() != 2) {
    printk("send_comp_avail() %d != 2\n", send_comp_avail());
    return false;
  }  
  *ICENET_SEND_COMP;
  if(send_comp_avail() != 1) {
    printk("send_comp_avail() %d != 1\n", send_comp_avail());
    return false;
  } 
  
  *ICENET_SEND_COMP;
  if(send_comp_avail() != 0) {
    printk("send_comp_avail() %d != 0\n", send_comp_avail());
    return false;
  } 
  
  return true;

}

void send_15(char index) {
  unsigned char msg[15] = {'a','b','c','d','e','a','b','c','d','e','a','b','c','d','\0'};
  msg[1] = index;
  uintptr_t addr = va2pa((void *)msg);
  
  uint64_t packet;
  uint64_t len = 15;
  packet = (len << 48) | (addr & 0xffffffffffffL);

  *ICENET_SEND_REQ = packet; 
}

bool test_recv() {
  
  if(recv_comp_avail() != 0) {
    printk("1 recv_com_avail() %d != 0\n", recv_comp_avail());
    return false;
  }
  if(recv_req_avail() != 15) {
    printk("2 recv_req_avail() %d != 15\n", recv_req_avail());
    return false;
  }

  unsigned char msg[15] = {'a','b','c','d','e','a','b','c','d','e','a','b','c','d','\0'};
  unsigned char msg1[15];
  unsigned char msg2[15];
  unsigned char msg3[15];
  unsigned char msg4[15];
  unsigned char msg5[15];
  unsigned char msg6[15];
  unsigned char msg7[15];
  unsigned char msg8[15];
  unsigned char msg9[15];
  unsigned char msg10[15];
  unsigned char msg11[15];
  unsigned char msg12[15];
  unsigned char msg13[15];
  unsigned char msg14[15];
  unsigned char msg15[15];
  unsigned char msg16[15];

  unsigned char* msgs[16];
  msgs[0] = msg1;
  msgs[1] = msg2;
  msgs[2] = msg3;
  msgs[3] = msg4;
  msgs[4] = msg5;
  msgs[5] = msg6;
  msgs[6] = msg7;
  msgs[7] = msg8;
  msgs[8] = msg9;
  msgs[9] = msg10;
  msgs[10] = msg11;
  msgs[11] = msg12;
  msgs[12] = msg13;
  msgs[13] = msg14;
  msgs[14] = msg15;
  msgs[15] = msg16;

  uintptr_t addr1 = va2pa((void *)msg1);
  uintptr_t addr2 = va2pa((void *)msg2);
  uintptr_t addr3 = va2pa((void *)msg3);
  uintptr_t addr4 = va2pa((void *)msg4);

  uintptr_t addr5 = va2pa((void *)msg5);
  uintptr_t addr6 = va2pa((void *)msg6);
  uintptr_t addr7 = va2pa((void *)msg7);
  uintptr_t addr8 = va2pa((void *)msg8);

  uintptr_t addr9 = va2pa((void *)msg9);
  uintptr_t addr10 = va2pa((void *)msg10);
  uintptr_t addr11 = va2pa((void *)msg11);
  uintptr_t addr12 = va2pa((void *)msg12);

  uintptr_t addr13 = va2pa((void *)msg13);
  uintptr_t addr14 = va2pa((void *)msg14);
  uintptr_t addr15 = va2pa((void *)msg15);
  uintptr_t addr16 = va2pa((void *)msg16);
  *ICENET_RECV_REQ = addr1;
  *ICENET_RECV_REQ = addr2;
  *ICENET_RECV_REQ = addr3;
  *ICENET_RECV_REQ = addr4;
  if(recv_req_avail() != 11) {
    printk("3 recv_req_avail() %d != 11\n", recv_req_avail());    
    return false;
  } 
  *ICENET_RECV_REQ = addr5;
  *ICENET_RECV_REQ = addr6;
  *ICENET_RECV_REQ = addr7;
  *ICENET_RECV_REQ = addr8;
  *ICENET_RECV_REQ = addr9;
  *ICENET_RECV_REQ = addr10;
  *ICENET_RECV_REQ = addr11;
  *ICENET_RECV_REQ = addr12;
  *ICENET_RECV_REQ = addr13;
  *ICENET_RECV_REQ = addr14;
  *ICENET_RECV_REQ = addr15;
  if(recv_req_avail() != 0) {
    printk("3.5 recv_req_avail() %d != 0\n", recv_req_avail());    
    return false;
  } 

  printk("Now use ospf 6*2=12 times, each time send 1500 abcde, then come back and Press Enter to continue\n");

  send_15('1');
  send_15('2');
  send_15('3');
  send_15('4');
  send_15('5');
  send_15('6');
  int qq = 1;
  for(int i = 0; i < 10000000; i++) {
    for(int j = 0; j < 5; j++) {
      qq = qq*i*j%2 + 1;
    }
  }    
  if(recv_comp_avail() != 12) {
    printk("4%d recv_com_avail() %d != 12\n",qq, recv_comp_avail());
    return false;
  }
  printk("Now use ospf 2*2=4 times, each time send 1500 abcde, then come back and Press Enter to continue");
  send_15('7');
  send_15('8');

  for(int i = 0; i < 10000000; i++) {
    for(int j = 0; j < 5; j++) {
      qq = qq*i*j%2 + 1;
    }
  } 
  if(recv_comp_avail() != 15) {
    printk("4.2 __%d recv_com_avail() %d != 15\n", qq, recv_comp_avail());
    return false;
  }

  for(int i = 0; i < 15; i++) {
    if(msgs[i][1] != 1 + i/2 + 48) {
      printk("msgs[%d] read wrongly in index = %s\n", i, msgs[i]);
      return false;
    }
    for(int j = 2; j < 15; j++) {
      if(msgs[i][j] != msg[j]) {
        printk("msgs[%d] read wrongly in contents = %s\n", i, msgs[i]);
        return false;
      }  
    }
  }  
  
  for(int i = 0; i < 15; i++) {
    int length = *ICENET_RECV_COMP;
    if(length != 15) {
      printk("Length received is wrong, %d != 15\n", length);
      return false;
    }
    if(recv_comp_avail() != 15-i-1) {
      printk("5 recv_com_avail() %d != %d\n", recv_comp_avail(), 15-i);
      return false;
    }
   if(recv_req_avail() != i+1) {
      printk("6 recv_req_avail() %d != %d\n", recv_req_avail(), i+1);
      return false;
    }
  }
  return true;
}
