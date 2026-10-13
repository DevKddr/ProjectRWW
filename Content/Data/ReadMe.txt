Content/Data 폴더 구조 안내
================================

카테고리별로 원본 엑셀 + 변환 스크립트를 하위 폴더로 분리했습니다.
변환 결과(JSON)는 전부 공용 output 폴더 하나에 모입니다 - 언리얼 C++ 코드가
Content/Data/output/*.json 경로를 그대로 참조하기 때문에, output 폴더 위치는
바꾸면 안 됩니다.

Content/Data/
├─ output/                        (공용, 전체 변환 결과 JSON)
│   ├─ weapons.json
│   ├─ items.json
│   └─ PlayerBaseStat.json
├─ weapons/
│   ├─ WeaponData.xlsx
│   ├─ WeaponDataConverter.py
│   └─ WeaponDataConverter.bat     (더블클릭 -> ../output 에 weapons.json 생성)
├─ items/
│   ├─ items.xlsx
│   ├─ ItemDataConverter.py
│   └─ ItemDataConverter.bat       (더블클릭 -> ../output 에 items.json 생성)
├─ player/
│   ├─ PlayerBaseStat.xlsx
│   ├─ player_base_stat_converter.py
│   └─ run_convert.bat             (더블클릭 -> ../output 에 PlayerBaseStat.json 생성)
├─ randombox/
│   └─ RandomBox.xlsx              (가챠/랜덤박스 시스템 재기획 예정 - 현재 변환 스크립트 없음)
└─ Content.lnk                     (Content 폴더 바로가기)

사용법
------
각 폴더의 .bat 파일을 더블클릭하면, 그 폴더 안의 엑셀을 읽어서
한 단계 위(..\output)에 JSON을 생성합니다. Python과 openpyxl 패키지가
설치되어 있어야 합니다 (pip install openpyxl).

엑셀 파일을 수정한 뒤에는 반드시 해당 폴더의 .bat을 다시 실행해서
output 폴더의 JSON을 갱신해야 게임에 반영됩니다.

Localization (아이템 이름/설명)
------------------------------
아이템 이름/설명은 Localization Dashboard 방식입니다. 소스 언어는 ko(한국어).
- items.xlsx 의 "Localization" 시트(Key/ko)가 원문(ko)의 원본입니다. 번역문(en 등)은 엑셀에 없고
  Localization Dashboard 에서만 관리합니다.
- ItemDataConverter.bat 을 실행하면 output 폴더에 아래 2개가 생성됩니다.
    items.json       : 이름/설명 대신 키(nameKey/descriptionKey)만 들어갑니다.
    ST_Items.csv     : StringTable 원문(ko). 언리얼 /Game/Core/Text/ST_Items 에 임포트합니다.
- 문장을 바꾸거나 키를 추가했다면: 컨버터 실행 -> ST_Items.csv 를 ST_Items 에 다시 임포트
  -> Localization Dashboard 에서 Gather -> 번역(en) 입력/확인 -> Compile.
- 게임 코드는 UItemDataManager::GetItemNameText / GetItemDescriptionText 로 현재 언어의 FText 를 얻습니다.
