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

Loot (지역별 아이템 스폰)
-------------------------
레벨의 스폰 포인트마다 아이템을 뽑아 바닥 픽업(AItemPickupActor)으로 스폰합니다. 서버에서 레이드 시작 시 1회.
- 레벨 디자이너가 하는 일: ALootRegion(박스 볼륨, RegionId 입력)과 ALootSpawnPoint(바닥 위치)를 배치.
  * 포인트는 자기가 들어 있는 지역의 RegionId를 따릅니다. 테이블을 포인트에 직접 지정하지 않습니다.
  * RegionId를 비웠거나, 어느 지역에도 안 들어 있거나, loot_tables.json 에 없는 RegionId 면 "Default" 를 씁니다.
  * 지역이 겹치면 Priority 가 큰 쪽, 같으면 부피가 작은 쪽이 이깁니다.
  * 예외적으로 포인트 하나만 다른 테이블을 쓰려면 ALootSpawnPoint.TableOverride 에 TableId 를 적습니다.
- output/loot_tables.json (현재 엑셀 컨버터 없이 직접 편집):
    tables  : TableId -> rarityWeights(등급) / categoryWeights(카테고리) / itemWeights(아이템 배수, 0=제외) /
              excludedItems / parent(상속: 적은 키만 덮어씀)
    regions : RegionId -> table(TableId), spawnChance(포인트 하나가 실제로 스폰될 확률 0~1)
    "Default" 지역과 그 테이블은 반드시 있어야 합니다.
  추첨: 등급 -> 카테고리 -> 아이템 3단계 가중치. 후보는 items.json 의 등급(rarityId)/카테고리에서 자동으로 가져옵니다.
- 아이템이 어떤 픽업 BP 로 놓일지는 Project Settings > ProjectRWW Loot Settings 의 PickupClassByItem
  (아이템 Index -> BP_Pickup_xxx)에서 정합니다. 매핑이 없으면 DefaultPickupClass, 그것도 없으면 스폰하지 않고 경고합니다.
- World Partition 맵: ALootRegion/ALootSpawnPoint 는 항상 로드되도록(Is Spatially Loaded = false) 기본값을 꺼 두었습니다.
  서버가 플레이어 없이 레이드 시작 시점에 스폰하기 때문입니다. 이 값을 켜면 스폰이 0개가 됩니다.
- 디버그 콘솔: rww.Loot.Respawn (다시 스폰), rww.Loot.Dump (지역별 포인트/스폰 수 로그)
