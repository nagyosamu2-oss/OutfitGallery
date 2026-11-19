#pragma once
namespace Gallery {
// Confirm on second release, not press, so a second-click drag can cancel.
struct PreviewPauseGesture {
 bool armed{},dragged{},previousDragged{};
 void Reset(){armed=dragged=previousDragged=false;}
 bool Update(bool enabled,bool hovered,bool pressed,bool doubleClicked,bool dragging,bool released,bool rightDown) {
  if(!enabled || rightDown){Reset();return false;}
  if(pressed){armed=hovered && doubleClicked && !previousDragged;dragged=false;}
  if(dragging)dragged=true;
  if(!released)return false;
  const bool toggle=armed && !dragged && hovered;
  previousDragged=dragged;armed=false;dragged=false;return toggle;
 }
};
}
