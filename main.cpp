#include <iostream>
#include <gccore.h>
#include <unistd.h>
#include <ogc/es.h>
#include <format>

extern "C" {
#include <libpatcher/libpatcher.h>
}

static void* xfb = nullptr;
static GXRModeObj* rmode = nullptr;

int main() {
  VIDEO_Init();

  rmode = VIDEO_GetPreferredMode(nullptr);
  xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
  console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
  VIDEO_Configure(rmode);
  VIDEO_SetNextFramebuffer(xfb);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();
  if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

  apply_patches();

  ISFS_Initialize();

  std::cout << std::endl;
  std::cout << std::endl;
  std::cout << std::endl;

  u32 num_owned{};
  int res = ES_GetNumOwnedTitles(&num_owned);
  if (res < 0) {
    std::cout << "Error getting owned title count" << std::endl;
    sleep(5);
    WII_ReturnToMenu();
  }

  std::cout << "Owned titles: " << num_owned << std::endl;

  u64* titles = reinterpret_cast<u64*>(aligned_alloc(32, num_owned*8));
  res = ES_GetOwnedTitles(titles, num_owned);
  if (res < 0) {
    std::cout << "Error getting owned titles" << std::endl;
    sleep(5);
    WII_ReturnToMenu();
  }

  for (u32 i = 0; i < num_owned; i++) {
    // Do the funny
    u32 size{};
    res = ES_GetStoredTMDSize(titles[i], &size);
    if (res == -106) {
      // Kill
      u32 num_views{};
      res = ES_GetNumTicketViews(titles[i], &num_views);
      if (res < 0) {
        std::cout << "Error getting number of ticket views for " << std::format("{:016X}", titles[i]) << std::endl;
        sleep(2);
        continue;
      }

      tikview* views = reinterpret_cast<tikview*>(aligned_alloc(32, sizeof(tikview)*num_views));
      res = ES_GetTicketViews(titles[i], views, num_views);
      if (res < 0) {
        std::cout << "Error getting ticket views for " << std::format("{:016X}", titles[i]) << std::endl;
        sleep(2);
        continue;
      }

      res = ES_DeleteTicket(views);
      if (res < 0) {
        std::cout << "Error deleting ticket for " << std::format("{:016X}", titles[i]) << " code: " << res << std::endl;
        sleep(2);
        continue;
      }
      std::cout << "Successfully deleted ticket " << std::format("{:016X}", titles[i]) << std::endl;
    } else if (res < 0) {
      std::cout << "Error getting tmd for " << std::format("{:016X}", titles[i]) << std::endl;
      std::cout << "Result: " << res << std::endl;
      sleep(2);
    }
  }

  sleep(5);

  return 0;
}
