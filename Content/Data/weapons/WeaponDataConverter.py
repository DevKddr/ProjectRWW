"""
카테고리별 아이템 원본 파일(WeaponData.xlsx 등) -> JSON 컨버터

카테고리 파일: 그 카테고리 아이템의 실제 스탯 원본 데이터.
출력물은 카테고리별로 분리된 JSON이다:
  - {category}.json : 카테고리별 아이템 리스트 (예: weapons.json). 파일명은 CATEGORY_FILE_NAMES 참조.

등급(Rarity)/가챠(랜덤박스, 추첨 테이블) 관련 변환은 더 이상 이 스크립트에서 다루지 않는다 -
가챠 시스템은 추후 재기획을 통해 별도로 다시 설계될 예정이라, RandomBox.xlsx 의존성과
rarities.json / gacha_tables.json 출력을 전부 제거했다.

새 카테고리(Skin, Consumable 등)를 추가하려면:
  1. 그 카테고리 아이템 원본 파일을 읽는 로더 함수를 만들어 CATEGORY_LOADERS 에 등록
  2. 출력 파일명을 CATEGORY_FILE_NAMES 에 등록 (안 하면 "{category.lower()}.json"으로 자동 생성)

사용법:
    python WeaponDataConverter.py WeaponData.xlsx ./output
    (카테고리 파일은 여러 개 추가 가능: ... WeaponData.xlsx SkinData.xlsx ./output)
"""
import sys
import json
from pathlib import Path
import openpyxl

# 발사 방식(FireMode)별 시트. 한 시트 안에 총기 종류(WeaponType)별 독립 테이블이 세로로 쌓여 있다.
# 시트 이름 = 그 시트에 들어갈 수 있는 FireMode 값이다(각 행의 FireMode 컬럼과 어긋나면 에러).
# 이 목록에 없는 시트(Legend 등)는 읽지 않는다.
WEAPON_SHEETS = ["Single", "Burst", "FullAuto"]


def _normalize_value(v):
    """엑셀 드롭다운 검증에 쓴 'TRUE'/'FALSE' 문자열을 실제 JSON bool로 변환.
    (그대로 두면 JSON에 문자열 "TRUE"로 나가서 언리얼의 bool 파싱이 깨짐)"""
    if isinstance(v, str) and v in ("TRUE", "FALSE"):
        return v == "TRUE"
    return v


def sheet_tables(ws):
    """시트 하나에서 테이블들을 읽는다. 테이블 한 개의 모양:
         [제목]   <- 첫 칸이 "[...]" 형태인 행 (표시용. 총기 종류 이름)
         헤더 행  <- 첫 칸이 "Index"
         타입 행  <- 건너뜀
         데이터 행들 (첫 칸이 비면 테이블 끝)
    테이블 사이는 빈 행으로 구분한다. (제목, [헤더:값 dict 리스트]) 튜플 리스트를 반환."""
    rows = list(ws.iter_rows(values_only=True))
    tables = []
    i = 0
    while i < len(rows):
        first = rows[i][0]
        if isinstance(first, str) and first.startswith("[") and first.endswith("]"):
            title = first[1:-1]
            if i + 2 >= len(rows) or rows[i + 1][0] != "Index":
                raise ValueError(f"{ws.title}!{i + 1}행: 제목 '{first}' 다음 행이 헤더(Index)가 아닙니다.")
            headers = rows[i + 1]
            i += 3  # 제목, 헤더, 타입 행
            data = []
            while i < len(rows) and rows[i][0] is not None:
                data.append({h: _normalize_value(v) for h, v in zip(headers, rows[i]) if h is not None})
                i += 1
            tables.append((title, data))
        else:
            i += 1
    return tables


# ---------------------------------------------------------------------------
# 카테고리별 로더. "이 카테고리 파일을 어떻게 읽어서 {index: item} 으로 만드는가"를 정의.
# 새 카테고리를 추가할 때 이 dict 에 함수 하나만 등록하면 된다.
# ---------------------------------------------------------------------------

def load_weapon_category(xlsx_path):
    """이름/설명/등급은 여기서 다루지 않는다 - items.json이 그 값들의 유일한 출처다
    (ItemDataManager: "무기를 포함한 모든 아이템의 표시 데이터"). 이 함수는 순수하게
    게임플레이 스탯(WeaponData.xlsx)만 만든다."""
    wb = openpyxl.load_workbook(xlsx_path, data_only=True)

    items = {}
    for sheet_name in WEAPON_SHEETS:
        if sheet_name not in wb.sheetnames:
            continue
        for title, rows in sheet_tables(wb[sheet_name]):
            for row in rows:
                index = row["Index"]
                if row.get("FireMode") != sheet_name:
                    raise ValueError(f"{sheet_name}/[{title}] {index}: FireMode='{row.get('FireMode')}' 가 시트 이름과 다릅니다.")
                if row.get("WeaponType") != title:
                    raise ValueError(f"{sheet_name}/[{title}] {index}: WeaponType='{row.get('WeaponType')}' 가 테이블 제목과 다릅니다.")
                if index in items:
                    raise ValueError(f"Index '{index}' 가 중복됩니다.")
                stats = {k: v for k, v in row.items() if k not in ("Index",)}
                items[index] = {
                    "index": index,
                    "weaponType": row["WeaponType"],
                    "stats": stats,
                }
    return items


# 카테고리 이름 -> 로더 함수. 앞으로 "Skin": load_skin_category 처럼 추가.
CATEGORY_LOADERS = {
    "Weapon": load_weapon_category,
}

# 카테고리 이름 -> 출력 파일명. 카테고리마다 독립된 JSON으로 분리해서 내보낸다.
# (Git 충돌 방지, 언리얼에서 필요한 카테고리만 선택적으로 로드하기 위함)
# 새 카테고리를 추가할 때 CATEGORY_LOADERS와 함께 이 dict에도 등록할 것.
CATEGORY_FILE_NAMES = {
    "Weapon": "weapons.json",
}


def main():
    if len(sys.argv) < 3:
        print("usage: python WeaponDataConverter.py <CategoryFile1.xlsx> [CategoryFile2.xlsx ...] <output_dir>")
        print("  (현재 지원 카테고리 파일: WeaponData.xlsx 형식 -> 'Weapon' 카테고리)")
        sys.exit(1)

    *category_paths, out_dir = sys.argv[1:]
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # 현재는 파일 하나 = Weapon 카테고리 하나로 단순 매핑한다.
    # 카테고리가 늘어나면 파일명 규칙이나 별도 인자로 카테고리를 지정하도록 확장하면 된다.
    category_items = {}
    for p in category_paths:
        category_items["Weapon"] = load_weapon_category(p)

    # 카테고리별 출력: {카테고리명}.json (예: weapons.json). 리스트(JSON 배열) 형태로 저장 -
    # 언리얼에서 FJsonObjectConverter::JsonArrayStringToUStruct로 그대로 파싱 가능하게 하기 위함.
    category_output_paths = {}
    for category, items in category_items.items():
        filename = CATEGORY_FILE_NAMES.get(category)
        if filename is None:
            # 새 카테고리를 등록할 때 CATEGORY_FILE_NAMES에 파일명을 안 넣은 경우를 위한 안전한 기본값.
            filename = f"{category.lower()}.json"
        path = out_dir / filename
        with open(path, "w", encoding="utf-8") as f:
            json.dump(list(items.values()), f, ensure_ascii=False, indent=2)
        category_output_paths[category] = path

    for category, path in category_output_paths.items():
        print(f"[OK] {path}  ({category} 아이템 {len(category_items[category])}개)")

    print("\n검증 통과: 경고 없음.")


if __name__ == "__main__":
    main()
