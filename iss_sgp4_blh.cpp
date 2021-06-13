/***********************************************************
  TLE から ISS の位置・速度を計算
  : 但し、座標系は BLH(Beta(Latitude), Lambda(Longitude), Height)
  : 指定 UT1 からの TEME 座標計算は iss_sgp4_teme

    DATE        AUTHOR       VERSION
    2021.05.27  mk-mode.com  1.00 新規作成

  Copyright(C) 2021 mk-mode.com All Rights Reserved.
  ---
  引数 : JST（日本標準時）
           書式：最大23桁の数字
                 （先頭から、西暦年(4), 月(2), 日(2), 時(2), 分(2), 秒(2),
                             1秒未満(9)（小数点以下9桁（ナノ秒）まで））
                 無指定なら現在(システム日時)と判断。
  ---
  MEMO:
    TEME: True Equator, Mean Equinox; 真赤道面平均春分点
     PEF: Pseudo Earth Fixed; 擬地球固定座標系
    ECEF: Earth Centered, Earth Fixed; 地球中心・地球固定直交座標系
***********************************************************/
#include "blh.hpp"
#include "eop.hpp"
#include "sgp4.hpp"
#include "time.hpp"
#include "tle.hpp"

#include <cstdlib>   // for EXIT_XXXX
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
  namespace ns = iss_sgp4_blh;
  std::string tm_str;            // time string
  unsigned int s_tm;             // size of time string
  int s_nsec;                    // size of nsec string
  int ret;                       // return of functions
  struct tm t = {};              // for work
  struct timespec jst;           // JST
  struct timespec utc;           // UTC
  struct timespec ut1;           // UT1
  struct timespec tai;           // TAI
  double pm_x;                   // 極運動(x)
  double pm_y;                   // 極運動(y)
  std::string lod_str;           // LOD 文字列
  double lod;                    // LOD
  std::vector<std::string> tle;  // TLE
  ns::Satellite sat;             // 衛星情報
  ns::PvTeme    teme;            // 位置・速度(TEME)
  ns::PvBlh     blh;             // 位置・速度(BLH)

  try {
    // 現在日時(UT1) 取得
    if (argc > 1) {
      // コマンドライン引数より取得
      tm_str = argv[1];
      s_tm = tm_str.size();
      if (s_tm > 23) { 
        std::cout << "[ERROR] Over 23-digits!" << std::endl;
        return EXIT_FAILURE;
      }
      s_nsec = s_tm - 14;
      std::istringstream is(tm_str);
      is >> std::get_time(&t, "%Y%m%d%H%M%S");
      jst.tv_sec  = mktime(&t);
      jst.tv_nsec = 0;
      if (s_tm > 14) {
        jst.tv_nsec = std::stod(
            tm_str.substr(14, s_nsec) + std::string(9 - s_nsec, '0'));
      }
    } else {
      // 現在日時の取得
      ret = std::timespec_get(&jst, TIME_UTC);
      if (ret != 1) {
        std::cout << "[ERROR] Could not get now time!" << std::endl;
        return EXIT_FAILURE;
      }
    }

    // EOP データ取得
    utc = ns::jst2utc(jst);
    ns::Eop o_e(utc);
    pm_x = o_e.pm_x;
    pm_y = o_e.pm_y;
    lod  = o_e.lod;
    ut1  = ns::utc2ut1(utc);
    tai  = ns::utc2tai(utc);
    std::cout << ns::gen_time_str(jst) << " JST" << std::endl;
    std::cout << ns::gen_time_str(utc) << " UTC" << std::endl;
    std::cout << ns::gen_time_str(ut1) << " UT1" << std::endl;
    std::cout << ns::gen_time_str(tai) << " TAI" << std::endl;

    // TLE 読み込み, gravconst 取得
    ns::Tle o_t(ut1);
    tle = o_t.get_tle();

    // ISS 初期位置・速度の取得
    ns::Sgp4 o_s(ut1, tle);
    sat = o_s.twoline2rv();

    // 指定 UT1 の ISS 位置・速度の取得
    teme = o_s.propagate(sat);

    // TEME -> BLH 変換
    ns::Blh o_b(ut1, tai, pm_x, pm_y, lod);
    blh = o_b.teme2blh(teme);

    // 結果出力
    std::cout << "---" << std::endl;
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "EOP: " << o_e.eop << std::endl;
    std::cout << "---" << std::endl;
    std::cout << "TLE: " << tle[0] << std::endl;
    std::cout << "     " << tle[1] << std::endl;
    std::cout << "---" << std::endl;
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "TEME: POS = ["
              << std::setw(16) << teme.r.x << ", "
              << std::setw(16) << teme.r.y << ", "
              << std::setw(16) << teme.r.z
              << "]" << std::endl;
    std::cout << "      VEL = ["
              << std::setw(16) << teme.v.x << ", "
              << std::setw(16) << teme.v.y << ", "
              << std::setw(16) << teme.v.z
              << "]" << std::endl;
    std::cout << "---" << std::endl;
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "WGS84(BLH):" << std::endl
              << "     BETA(Latitude) = " << std::setw(9) << blh.r.b
              << " °" << std::endl
              << "  LAMBDA(Longitude) = " << std::setw(9) << blh.r.l
              << " °" << std::endl
              << "             HEIGHT = " << std::setw(9) << blh.r.h / 1000.0
              << " km" << std::endl
              << "           VELOCITY = " << std::setw(9) << blh.v
              << " km/s" << std::endl;
  } catch (...) {
      std::cerr << "EXCEPTION!" << std::endl;
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

