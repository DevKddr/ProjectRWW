"""
items.xlsx -> items.json 컨버터.

WeaponDataConverter.py와 같은 패턴을 쓴다:
  - "Items" 시트: 1행=헤더, 2행=타입, 3행부터 데이터.
  - "Localization" 시트: 1행=헤더(Key/ko/en), 2행부터 데이터 (타입행 없음).
    NameKey/DescKey는 이 시트에서 실제 {ko,en} 값으로 치환되어 JSON에 들어가고,
    NameKey/DescKey라는 키 자체는 출력물에 남지 않는다.

출력은 items.json 하나(JSON 배열)이며, 언리얼에서
FJsonObjectConverter::JsonArrayStringToUStruct로 그대로 파싱 가능한 형태다.
FItemData.Name/Description이 FLocalizedPair({Ko,En}) 타입이어야 이 출력물이
정상적으로 파싱된다 - FString 타입인 채로 이 스크립트를 쓰면 파싱이 깨진다.

사용법:
    python ItemDataConverter.py items.xlsx ./output
"""
import sys
import json
from pathlib import Path
import openpyxl


def _normalize_value(v):
    """엑셀 드롭다운 검증에 쓴 'TRUE'/'FALSE' 문자열을 실제 JSON bool로 변환."""
    if isinstance(v, str) and v in ("TRUE", "FALSE"):
        return v == "TRUE"
    return v


def sheet_rows(ws, has_type_row=True):
    """1행=헤더, (있다면) 2행=타입, 그 다음부터 데이터. 헤더:값 dict 리스트로 반환."""
    rows = list(ws.iter_rows(values_only=True))
    min_rows = 3 if has_type_row else 2
    if len(rows) < min_rows:
        return []
    headers = rows[0]
    data_start = 2 if has_type_row else 1
    out = []
    for row in rows[data_start:]:
        if row[0] is None:
            continue
        out.append({h: _normalize_value(v) for h, v in zip(headers, row)})
    return out


def load_items(xlsx_path):
    wb = openpyxl.load_workbook(xlsx_path, data_only=True)

    localization = {}
    for row in sheet_rows(wb["Localization"], has_type_row=False):
        localization[row["Key"]] = {"ko": row["ko"], "en": row["en"]}

    def localize(key):
        entry = localization.get(key)
        if entry is None:
            print(f"[WARN] Localization 시트에 없는 키: {key} (ko/en 자리에 키 값이 그대로 들어감)")
            return {"ko": key, "en": key}
        return entry

    items = []
    for row in sheet_rows(wb["Items"]):
        icon_path = row.get("IconPath") or ""
        items.append({
            "index": row["Index"],
            "category": row["Category"],
            "name": localize(row["NameKey"]),
            "description": localize(row["DescKey"]),
            "rarityId": row["RarityId"],
            "iconPath": icon_path,
            "actorClassPath": row["ActorClassPath"],
            "equipTime": float(row["EquipTime"]),
            "primarySkillCooldown": float(row["PrimarySkillCooldown"]),
            "primarySkillManaCost": float(row["PrimarySkillManaCost"]),
            "primarySkillCastTime": float(row["PrimarySkillCastTime"]),
            "secondarySkillCooldown": float(row["SecondarySkillCooldown"]),
            "secondarySkillManaCost": float(row["SecondarySkillManaCost"]),
            "secondarySkillCastTime": float(row["SecondarySkillCastTime"]),
        })
    return items


def main():
    if len(sys.argv) != 3:
        print("usage: python ItemDataConverter.py <items.xlsx> <output_dir>")
        sys.exit(1)

    xlsx_path = Path(sys.argv[1])
    out_dir = Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)

    items = load_items(xlsx_path)

    out_path = out_dir / "items.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(items, f, ensure_ascii=False, indent=2)

    print(f"[OK] {out_path}  (아이템 {len(items)}개)")


if __name__ == "__main__":
    main()
