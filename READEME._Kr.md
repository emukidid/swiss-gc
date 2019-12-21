# 스위스(Swiss)

## 목차
- [목적](#purpose)
 - [주요 특징](#main-features)
 - [요구사항](#requirements)
 - [사용법](#usage)

## 목적
스위스는 게임큐브를 위한 올인원 홈브류 도구를 목표로 합니다.

###주요 특징
**다음 장치를 탐색할 수 있습니다.**
- SDGecko를 통한 SD/SDHC/SDXC 카드
- DVD 드라이브를 통한 DVD (-/+ R)
- Qoob 프로 플래시 메모리
- USB Gecko 원격 파일 저장소
- Wasp/WKF를 통한 SD/SDHC
- BBA를 통한 삼바
- Wode 쥬크박스
- IDE-EXI
- 메모리 카드

###요구사항
- 컨트롤러가 있는 게임큐브
- [홈브류로 부팅 하는 방법](https://gc-forever.com/wiki/index.php?title=Booting_Homebrew)

### 사용법
1. [최신 스위스 버전을 다운로드](https://github.com/emukidid/swiss-gc/releases)하고 내용을 추출하십시오.
2. DOL 폴더에 있는 압축된 Swiss DOL 파일을 홈브류를 부팅하는데 사용하는 장치/매체에 복사합니다.
3. 스위스를 시작하고 기기를 탐색하고 DOL 또는 GCM을 로드하십시오!

위의 단계가 작동하지 않으면 압축되지 않은 Swiss DOL 파일을 사용해보십시오.

## 스위스 탐색
### 컨트롤
<table>
    <tr style="font-weight:bold">
        <td>컨트롤</td>
        <td>동작</td>
    </tr>
    <tr>
        <td>왼쪽 조이스틱 또는 D-패드</td>
        <td>UI를 통해 탐색</td>
    </tr>
    <tr>
        <td>A</td>
        <td>선택</td>
    </tr>
    <tr>
        <td>B</td>
        <td>진입/하단 메뉴 종료</td>
    </tr>
</table>

###스위스 UI
- 맨 위 제목에는 스위스의 버전 번호, 커밋 번호 및 개정 번호 표시.
- 왼쪽 창에는 현재 사용중인 장치 표시.
- 가장 큰 부분은 파일 및 폴더를 탐색할 수 있는 스위스 파일 브라우저. 모든 폴더의 맨 위에는 ..옵션이 있으며 이 옵션을 선택하면 폴더가 백업.
- 아래쪽 창 (왼쪽부터) :
   - 장치 선택
   - 전체 설정, 고급 설정 및 현재 게임 설정
   - 시스템 정보, 장치 정보 및 크레딧
   - 파일 시스템의 맨 위로 돌아 가기
   - 게임큐브 재시작
