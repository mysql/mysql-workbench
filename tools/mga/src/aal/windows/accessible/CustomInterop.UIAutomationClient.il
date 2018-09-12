
//  Microsoft (R) .NET Framework IL Disassembler.  Version 3.5.30729.1
//  Copyright (c) Microsoft Corporation.  All rights reserved.



// Metadata version: v2.0.50727
.assembly extern mscorlib
{
  .publickeytoken = (B7 7A 5C 56 19 34 E0 89 )                         // .z\V.4..
  .ver 2:0:0:0
}
.assembly Interop.UIAutomationClient
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.ImportedFromTypeLibAttribute::.ctor(string) = ( 01 00 12 55 49 41 75 74 6F 6D 61 74 69 6F 6E 43   // ...UIAutomationC
                                                                                                                 6C 69 65 6E 74 00 00 )                            // lient..
  .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibVersionAttribute::.ctor(int32,
                                                                                                int32) = ( 01 00 01 00 00 00 00 00 00 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 39 34 34 64 65 30 38 33 2D 38 66 62 38   // ..$944de083-8fb8
                                                                                                  2D 34 35 63 66 2D 62 63 62 37 2D 63 34 37 37 61   // -45cf-bcb7-c477a
                                                                                                  63 62 32 66 38 39 37 00 00 )                      // cb2f897..
  .hash algorithm 0x00008004
  .ver 1:0:0:0
}
.module UIAutomationClient.dll
// MVID: {CC084CC4-ABC8-4385-A01C-D579FF672863}
.imagebase 0x00400000
.file alignment 0x00001000
.stackreserve 0x00100000
.subsystem 0x0003       // WINDOWS_CUI
.corflags 0x00000001    //  ILONLY
// Image base: 0x005F0000


// =============== CLASS MEMBERS DECLARATION ===================

.class public abstract auto ansi UIAutomationClient.UIA_PatternIds
       extends [mscorlib]System.Object
{
  .field public static literal int32 UIA_InvokePatternId = int32(0x00002710)
  .field public static literal int32 UIA_SelectionPatternId = int32(0x00002711)
  .field public static literal int32 UIA_ValuePatternId = int32(0x00002712)
  .field public static literal int32 UIA_RangeValuePatternId = int32(0x00002713)
  .field public static literal int32 UIA_ScrollPatternId = int32(0x00002714)
  .field public static literal int32 UIA_ExpandCollapsePatternId = int32(0x00002715)
  .field public static literal int32 UIA_GridPatternId = int32(0x00002716)
  .field public static literal int32 UIA_GridItemPatternId = int32(0x00002717)
  .field public static literal int32 UIA_MultipleViewPatternId = int32(0x00002718)
  .field public static literal int32 UIA_WindowPatternId = int32(0x00002719)
  .field public static literal int32 UIA_SelectionItemPatternId = int32(0x0000271A)
  .field public static literal int32 UIA_DockPatternId = int32(0x0000271B)
  .field public static literal int32 UIA_TablePatternId = int32(0x0000271C)
  .field public static literal int32 UIA_TableItemPatternId = int32(0x0000271D)
  .field public static literal int32 UIA_TextPatternId = int32(0x0000271E)
  .field public static literal int32 UIA_TogglePatternId = int32(0x0000271F)
  .field public static literal int32 UIA_TransformPatternId = int32(0x00002720)
  .field public static literal int32 UIA_ScrollItemPatternId = int32(0x00002721)
  .field public static literal int32 UIA_LegacyIAccessiblePatternId = int32(0x00002722)
  .field public static literal int32 UIA_ItemContainerPatternId = int32(0x00002723)
  .field public static literal int32 UIA_VirtualizedItemPatternId = int32(0x00002724)
  .field public static literal int32 UIA_SynchronizedInputPatternId = int32(0x00002725)
} // end of class UIAutomationClient.UIA_PatternIds

.class public abstract auto ansi UIAutomationClient.UIA_EventIds
       extends [mscorlib]System.Object
{
  .field public static literal int32 UIA_ToolTipOpenedEventId = int32(0x00004E20)
  .field public static literal int32 UIA_ToolTipClosedEventId = int32(0x00004E21)
  .field public static literal int32 UIA_StructureChangedEventId = int32(0x00004E22)
  .field public static literal int32 UIA_MenuOpenedEventId = int32(0x00004E23)
  .field public static literal int32 UIA_AutomationPropertyChangedEventId = int32(0x00004E24)
  .field public static literal int32 UIA_AutomationFocusChangedEventId = int32(0x00004E25)
  .field public static literal int32 UIA_AsyncContentLoadedEventId = int32(0x00004E26)
  .field public static literal int32 UIA_MenuClosedEventId = int32(0x00004E27)
  .field public static literal int32 UIA_LayoutInvalidatedEventId = int32(0x00004E28)
  .field public static literal int32 UIA_Invoke_InvokedEventId = int32(0x00004E29)
  .field public static literal int32 UIA_SelectionItem_ElementAddedToSelectionEventId = int32(0x00004E2A)
  .field public static literal int32 UIA_SelectionItem_ElementRemovedFromSelectionEventId = int32(0x00004E2B)
  .field public static literal int32 UIA_SelectionItem_ElementSelectedEventId = int32(0x00004E2C)
  .field public static literal int32 UIA_Selection_InvalidatedEventId = int32(0x00004E2D)
  .field public static literal int32 UIA_Text_TextSelectionChangedEventId = int32(0x00004E2E)
  .field public static literal int32 UIA_Text_TextChangedEventId = int32(0x00004E2F)
  .field public static literal int32 UIA_Window_WindowOpenedEventId = int32(0x00004E30)
  .field public static literal int32 UIA_Window_WindowClosedEventId = int32(0x00004E31)
  .field public static literal int32 UIA_MenuModeStartEventId = int32(0x00004E32)
  .field public static literal int32 UIA_MenuModeEndEventId = int32(0x00004E33)
  .field public static literal int32 UIA_InputReachedTargetEventId = int32(0x00004E34)
  .field public static literal int32 UIA_InputReachedOtherElementEventId = int32(0x00004E35)
  .field public static literal int32 UIA_InputDiscardedEventId = int32(0x00004E36)
} // end of class UIAutomationClient.UIA_EventIds

.class public abstract auto ansi UIAutomationClient.UIA_PropertyIds
       extends [mscorlib]System.Object
{
  .field public static literal int32 UIA_RuntimeIdPropertyId = int32(0x00007530)
  .field public static literal int32 UIA_BoundingRectanglePropertyId = int32(0x00007531)
  .field public static literal int32 UIA_ProcessIdPropertyId = int32(0x00007532)
  .field public static literal int32 UIA_ControlTypePropertyId = int32(0x00007533)
  .field public static literal int32 UIA_LocalizedControlTypePropertyId = int32(0x00007534)
  .field public static literal int32 UIA_NamePropertyId = int32(0x00007535)
  .field public static literal int32 UIA_AcceleratorKeyPropertyId = int32(0x00007536)
  .field public static literal int32 UIA_AccessKeyPropertyId = int32(0x00007537)
  .field public static literal int32 UIA_HasKeyboardFocusPropertyId = int32(0x00007538)
  .field public static literal int32 UIA_IsKeyboardFocusablePropertyId = int32(0x00007539)
  .field public static literal int32 UIA_IsEnabledPropertyId = int32(0x0000753A)
  .field public static literal int32 UIA_AutomationIdPropertyId = int32(0x0000753B)
  .field public static literal int32 UIA_ClassNamePropertyId = int32(0x0000753C)
  .field public static literal int32 UIA_HelpTextPropertyId = int32(0x0000753D)
  .field public static literal int32 UIA_ClickablePointPropertyId = int32(0x0000753E)
  .field public static literal int32 UIA_CulturePropertyId = int32(0x0000753F)
  .field public static literal int32 UIA_IsControlElementPropertyId = int32(0x00007540)
  .field public static literal int32 UIA_IsContentElementPropertyId = int32(0x00007541)
  .field public static literal int32 UIA_LabeledByPropertyId = int32(0x00007542)
  .field public static literal int32 UIA_IsPasswordPropertyId = int32(0x00007543)
  .field public static literal int32 UIA_NativeWindowHandlePropertyId = int32(0x00007544)
  .field public static literal int32 UIA_ItemTypePropertyId = int32(0x00007545)
  .field public static literal int32 UIA_IsOffscreenPropertyId = int32(0x00007546)
  .field public static literal int32 UIA_OrientationPropertyId = int32(0x00007547)
  .field public static literal int32 UIA_FrameworkIdPropertyId = int32(0x00007548)
  .field public static literal int32 UIA_IsRequiredForFormPropertyId = int32(0x00007549)
  .field public static literal int32 UIA_ItemStatusPropertyId = int32(0x0000754A)
  .field public static literal int32 UIA_IsDockPatternAvailablePropertyId = int32(0x0000754B)
  .field public static literal int32 UIA_IsExpandCollapsePatternAvailablePropertyId = int32(0x0000754C)
  .field public static literal int32 UIA_IsGridItemPatternAvailablePropertyId = int32(0x0000754D)
  .field public static literal int32 UIA_IsGridPatternAvailablePropertyId = int32(0x0000754E)
  .field public static literal int32 UIA_IsInvokePatternAvailablePropertyId = int32(0x0000754F)
  .field public static literal int32 UIA_IsMultipleViewPatternAvailablePropertyId = int32(0x00007550)
  .field public static literal int32 UIA_IsRangeValuePatternAvailablePropertyId = int32(0x00007551)
  .field public static literal int32 UIA_IsScrollPatternAvailablePropertyId = int32(0x00007552)
  .field public static literal int32 UIA_IsScrollItemPatternAvailablePropertyId = int32(0x00007553)
  .field public static literal int32 UIA_IsSelectionItemPatternAvailablePropertyId = int32(0x00007554)
  .field public static literal int32 UIA_IsSelectionPatternAvailablePropertyId = int32(0x00007555)
  .field public static literal int32 UIA_IsTablePatternAvailablePropertyId = int32(0x00007556)
  .field public static literal int32 UIA_IsTableItemPatternAvailablePropertyId = int32(0x00007557)
  .field public static literal int32 UIA_IsTextPatternAvailablePropertyId = int32(0x00007558)
  .field public static literal int32 UIA_IsTogglePatternAvailablePropertyId = int32(0x00007559)
  .field public static literal int32 UIA_IsTransformPatternAvailablePropertyId = int32(0x0000755A)
  .field public static literal int32 UIA_IsValuePatternAvailablePropertyId = int32(0x0000755B)
  .field public static literal int32 UIA_IsWindowPatternAvailablePropertyId = int32(0x0000755C)
  .field public static literal int32 UIA_ValueValuePropertyId = int32(0x0000755D)
  .field public static literal int32 UIA_ValueIsReadOnlyPropertyId = int32(0x0000755E)
  .field public static literal int32 UIA_RangeValueValuePropertyId = int32(0x0000755F)
  .field public static literal int32 UIA_RangeValueIsReadOnlyPropertyId = int32(0x00007560)
  .field public static literal int32 UIA_RangeValueMinimumPropertyId = int32(0x00007561)
  .field public static literal int32 UIA_RangeValueMaximumPropertyId = int32(0x00007562)
  .field public static literal int32 UIA_RangeValueLargeChangePropertyId = int32(0x00007563)
  .field public static literal int32 UIA_RangeValueSmallChangePropertyId = int32(0x00007564)
  .field public static literal int32 UIA_ScrollHorizontalScrollPercentPropertyId = int32(0x00007565)
  .field public static literal int32 UIA_ScrollHorizontalViewSizePropertyId = int32(0x00007566)
  .field public static literal int32 UIA_ScrollVerticalScrollPercentPropertyId = int32(0x00007567)
  .field public static literal int32 UIA_ScrollVerticalViewSizePropertyId = int32(0x00007568)
  .field public static literal int32 UIA_ScrollHorizontallyScrollablePropertyId = int32(0x00007569)
  .field public static literal int32 UIA_ScrollVerticallyScrollablePropertyId = int32(0x0000756A)
  .field public static literal int32 UIA_SelectionSelectionPropertyId = int32(0x0000756B)
  .field public static literal int32 UIA_SelectionCanSelectMultiplePropertyId = int32(0x0000756C)
  .field public static literal int32 UIA_SelectionIsSelectionRequiredPropertyId = int32(0x0000756D)
  .field public static literal int32 UIA_GridRowCountPropertyId = int32(0x0000756E)
  .field public static literal int32 UIA_GridColumnCountPropertyId = int32(0x0000756F)
  .field public static literal int32 UIA_GridItemRowPropertyId = int32(0x00007570)
  .field public static literal int32 UIA_GridItemColumnPropertyId = int32(0x00007571)
  .field public static literal int32 UIA_GridItemRowSpanPropertyId = int32(0x00007572)
  .field public static literal int32 UIA_GridItemColumnSpanPropertyId = int32(0x00007573)
  .field public static literal int32 UIA_GridItemContainingGridPropertyId = int32(0x00007574)
  .field public static literal int32 UIA_DockDockPositionPropertyId = int32(0x00007575)
  .field public static literal int32 UIA_ExpandCollapseExpandCollapseStatePropertyId = int32(0x00007576)
  .field public static literal int32 UIA_MultipleViewCurrentViewPropertyId = int32(0x00007577)
  .field public static literal int32 UIA_MultipleViewSupportedViewsPropertyId = int32(0x00007578)
  .field public static literal int32 UIA_WindowCanMaximizePropertyId = int32(0x00007579)
  .field public static literal int32 UIA_WindowCanMinimizePropertyId = int32(0x0000757A)
  .field public static literal int32 UIA_WindowWindowVisualStatePropertyId = int32(0x0000757B)
  .field public static literal int32 UIA_WindowWindowInteractionStatePropertyId = int32(0x0000757C)
  .field public static literal int32 UIA_WindowIsModalPropertyId = int32(0x0000757D)
  .field public static literal int32 UIA_WindowIsTopmostPropertyId = int32(0x0000757E)
  .field public static literal int32 UIA_SelectionItemIsSelectedPropertyId = int32(0x0000757F)
  .field public static literal int32 UIA_SelectionItemSelectionContainerPropertyId = int32(0x00007580)
  .field public static literal int32 UIA_TableRowHeadersPropertyId = int32(0x00007581)
  .field public static literal int32 UIA_TableColumnHeadersPropertyId = int32(0x00007582)
  .field public static literal int32 UIA_TableRowOrColumnMajorPropertyId = int32(0x00007583)
  .field public static literal int32 UIA_TableItemRowHeaderItemsPropertyId = int32(0x00007584)
  .field public static literal int32 UIA_TableItemColumnHeaderItemsPropertyId = int32(0x00007585)
  .field public static literal int32 UIA_ToggleToggleStatePropertyId = int32(0x00007586)
  .field public static literal int32 UIA_TransformCanMovePropertyId = int32(0x00007587)
  .field public static literal int32 UIA_TransformCanResizePropertyId = int32(0x00007588)
  .field public static literal int32 UIA_TransformCanRotatePropertyId = int32(0x00007589)
  .field public static literal int32 UIA_IsLegacyIAccessiblePatternAvailablePropertyId = int32(0x0000758A)
  .field public static literal int32 UIA_LegacyIAccessibleChildIdPropertyId = int32(0x0000758B)
  .field public static literal int32 UIA_LegacyIAccessibleNamePropertyId = int32(0x0000758C)
  .field public static literal int32 UIA_LegacyIAccessibleValuePropertyId = int32(0x0000758D)
  .field public static literal int32 UIA_LegacyIAccessibleDescriptionPropertyId = int32(0x0000758E)
  .field public static literal int32 UIA_LegacyIAccessibleRolePropertyId = int32(0x0000758F)
  .field public static literal int32 UIA_LegacyIAccessibleStatePropertyId = int32(0x00007590)
  .field public static literal int32 UIA_LegacyIAccessibleHelpPropertyId = int32(0x00007591)
  .field public static literal int32 UIA_LegacyIAccessibleKeyboardShortcutPropertyId = int32(0x00007592)
  .field public static literal int32 UIA_LegacyIAccessibleSelectionPropertyId = int32(0x00007593)
  .field public static literal int32 UIA_LegacyIAccessibleDefaultActionPropertyId = int32(0x00007594)
  .field public static literal int32 UIA_AriaRolePropertyId = int32(0x00007595)
  .field public static literal int32 UIA_AriaPropertiesPropertyId = int32(0x00007596)
  .field public static literal int32 UIA_IsDataValidForFormPropertyId = int32(0x00007597)
  .field public static literal int32 UIA_ControllerForPropertyId = int32(0x00007598)
  .field public static literal int32 UIA_DescribedByPropertyId = int32(0x00007599)
  .field public static literal int32 UIA_FlowsToPropertyId = int32(0x0000759A)
  .field public static literal int32 UIA_ProviderDescriptionPropertyId = int32(0x0000759B)
  .field public static literal int32 UIA_IsItemContainerPatternAvailablePropertyId = int32(0x0000759C)
  .field public static literal int32 UIA_IsVirtualizedItemPatternAvailablePropertyId = int32(0x0000759D)
  .field public static literal int32 UIA_IsSynchronizedInputPatternAvailablePropertyId = int32(0x0000759E)
} // end of class UIAutomationClient.UIA_PropertyIds

.class public abstract auto ansi UIAutomationClient.UIA_TextAttributeIds
       extends [mscorlib]System.Object
{
  .field public static literal int32 UIA_AnimationStyleAttributeId = int32(0x00009C40)
  .field public static literal int32 UIA_BackgroundColorAttributeId = int32(0x00009C41)
  .field public static literal int32 UIA_BulletStyleAttributeId = int32(0x00009C42)
  .field public static literal int32 UIA_CapStyleAttributeId = int32(0x00009C43)
  .field public static literal int32 UIA_CultureAttributeId = int32(0x00009C44)
  .field public static literal int32 UIA_FontNameAttributeId = int32(0x00009C45)
  .field public static literal int32 UIA_FontSizeAttributeId = int32(0x00009C46)
  .field public static literal int32 UIA_FontWeightAttributeId = int32(0x00009C47)
  .field public static literal int32 UIA_ForegroundColorAttributeId = int32(0x00009C48)
  .field public static literal int32 UIA_HorizontalTextAlignmentAttributeId = int32(0x00009C49)
  .field public static literal int32 UIA_IndentationFirstLineAttributeId = int32(0x00009C4A)
  .field public static literal int32 UIA_IndentationLeadingAttributeId = int32(0x00009C4B)
  .field public static literal int32 UIA_IndentationTrailingAttributeId = int32(0x00009C4C)
  .field public static literal int32 UIA_IsHiddenAttributeId = int32(0x00009C4D)
  .field public static literal int32 UIA_IsItalicAttributeId = int32(0x00009C4E)
  .field public static literal int32 UIA_IsReadOnlyAttributeId = int32(0x00009C4F)
  .field public static literal int32 UIA_IsSubscriptAttributeId = int32(0x00009C50)
  .field public static literal int32 UIA_IsSuperscriptAttributeId = int32(0x00009C51)
  .field public static literal int32 UIA_MarginBottomAttributeId = int32(0x00009C52)
  .field public static literal int32 UIA_MarginLeadingAttributeId = int32(0x00009C53)
  .field public static literal int32 UIA_MarginTopAttributeId = int32(0x00009C54)
  .field public static literal int32 UIA_MarginTrailingAttributeId = int32(0x00009C55)
  .field public static literal int32 UIA_OutlineStylesAttributeId = int32(0x00009C56)
  .field public static literal int32 UIA_OverlineColorAttributeId = int32(0x00009C57)
  .field public static literal int32 UIA_OverlineStyleAttributeId = int32(0x00009C58)
  .field public static literal int32 UIA_StrikethroughColorAttributeId = int32(0x00009C59)
  .field public static literal int32 UIA_StrikethroughStyleAttributeId = int32(0x00009C5A)
  .field public static literal int32 UIA_TabsAttributeId = int32(0x00009C5B)
  .field public static literal int32 UIA_TextFlowDirectionsAttributeId = int32(0x00009C5C)
  .field public static literal int32 UIA_UnderlineColorAttributeId = int32(0x00009C5D)
  .field public static literal int32 UIA_UnderlineStyleAttributeId = int32(0x00009C5E)
} // end of class UIAutomationClient.UIA_TextAttributeIds

.class public abstract auto ansi UIAutomationClient.UIA_ControlTypeIds
       extends [mscorlib]System.Object
{
  .field public static literal int32 UIA_ButtonControlTypeId = int32(0x0000C350)
  .field public static literal int32 UIA_CalendarControlTypeId = int32(0x0000C351)
  .field public static literal int32 UIA_CheckBoxControlTypeId = int32(0x0000C352)
  .field public static literal int32 UIA_ComboBoxControlTypeId = int32(0x0000C353)
  .field public static literal int32 UIA_EditControlTypeId = int32(0x0000C354)
  .field public static literal int32 UIA_HyperlinkControlTypeId = int32(0x0000C355)
  .field public static literal int32 UIA_ImageControlTypeId = int32(0x0000C356)
  .field public static literal int32 UIA_ListItemControlTypeId = int32(0x0000C357)
  .field public static literal int32 UIA_ListControlTypeId = int32(0x0000C358)
  .field public static literal int32 UIA_MenuControlTypeId = int32(0x0000C359)
  .field public static literal int32 UIA_MenuBarControlTypeId = int32(0x0000C35A)
  .field public static literal int32 UIA_MenuItemControlTypeId = int32(0x0000C35B)
  .field public static literal int32 UIA_ProgressBarControlTypeId = int32(0x0000C35C)
  .field public static literal int32 UIA_RadioButtonControlTypeId = int32(0x0000C35D)
  .field public static literal int32 UIA_ScrollBarControlTypeId = int32(0x0000C35E)
  .field public static literal int32 UIA_SliderControlTypeId = int32(0x0000C35F)
  .field public static literal int32 UIA_SpinnerControlTypeId = int32(0x0000C360)
  .field public static literal int32 UIA_StatusBarControlTypeId = int32(0x0000C361)
  .field public static literal int32 UIA_TabControlTypeId = int32(0x0000C362)
  .field public static literal int32 UIA_TabItemControlTypeId = int32(0x0000C363)
  .field public static literal int32 UIA_TextControlTypeId = int32(0x0000C364)
  .field public static literal int32 UIA_ToolBarControlTypeId = int32(0x0000C365)
  .field public static literal int32 UIA_ToolTipControlTypeId = int32(0x0000C366)
  .field public static literal int32 UIA_TreeControlTypeId = int32(0x0000C367)
  .field public static literal int32 UIA_TreeItemControlTypeId = int32(0x0000C368)
  .field public static literal int32 UIA_CustomControlTypeId = int32(0x0000C369)
  .field public static literal int32 UIA_GroupControlTypeId = int32(0x0000C36A)
  .field public static literal int32 UIA_ThumbControlTypeId = int32(0x0000C36B)
  .field public static literal int32 UIA_DataGridControlTypeId = int32(0x0000C36C)
  .field public static literal int32 UIA_DataItemControlTypeId = int32(0x0000C36D)
  .field public static literal int32 UIA_DocumentControlTypeId = int32(0x0000C36E)
  .field public static literal int32 UIA_SplitButtonControlTypeId = int32(0x0000C36F)
  .field public static literal int32 UIA_WindowControlTypeId = int32(0x0000C370)
  .field public static literal int32 UIA_PaneControlTypeId = int32(0x0000C371)
  .field public static literal int32 UIA_HeaderControlTypeId = int32(0x0000C372)
  .field public static literal int32 UIA_HeaderItemControlTypeId = int32(0x0000C373)
  .field public static literal int32 UIA_TableControlTypeId = int32(0x0000C374)
  .field public static literal int32 UIA_TitleBarControlTypeId = int32(0x0000C375)
  .field public static literal int32 UIA_SeparatorControlTypeId = int32(0x0000C376)
} // end of class UIAutomationClient.UIA_ControlTypeIds

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationElement
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 44 32 32 31 30 38 41 41 2D 38 41 43 35   // ..$D22108AA-8AC5
                                                                                                  2D 34 39 41 35 2D 38 33 37 42 2D 33 37 42 42 42   // -49A5-837B-37BBB
                                                                                                  33 44 37 35 39 31 45 00 00 )                      // 3D7591E..
  .method public hidebysig newslot abstract virtual 
          instance void  SetFocus() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::SetFocus

  .method public hidebysig newslot abstract virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray int) 
          GetRuntimeId() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetRuntimeId

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          FindFirst([in] valuetype UIAutomationClient.TreeScope scope,
                    [in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::FindFirst

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          FindAll([in] valuetype UIAutomationClient.TreeScope scope,
                  [in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::FindAll

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          FindFirstBuildCache([in] valuetype UIAutomationClient.TreeScope scope,
                              [in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition,
                              [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::FindFirstBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          FindAllBuildCache([in] valuetype UIAutomationClient.TreeScope scope,
                            [in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition,
                            [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::FindAllBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          BuildUpdatedCache([in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::BuildUpdatedCache

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( struct) 
          GetCurrentPropertyValue([in] int32 propertyId) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCurrentPropertyValue

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( struct) 
          GetCurrentPropertyValueEx([in] int32 propertyId,
                                    [in] int32 ignoreDefaultValue) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCurrentPropertyValueEx

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( struct) 
          GetCachedPropertyValue([in] int32 propertyId) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCachedPropertyValue

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( struct) 
          GetCachedPropertyValueEx([in] int32 propertyId,
                                   [in] int32 ignoreDefaultValue) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCachedPropertyValueEx

  .method public hidebysig newslot abstract virtual 
          instance native int  GetCurrentPatternAs([in] int32 patternId,
                                                   [in] valuetype [mscorlib]System.Guid& riid) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCurrentPatternAs

  .method public hidebysig newslot abstract virtual 
          instance native int  GetCachedPatternAs([in] int32 patternId,
                                                  [in] valuetype [mscorlib]System.Guid& riid) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCachedPatternAs

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( iunknown ) 
          GetCurrentPattern([in] int32 patternId) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCurrentPattern

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( iunknown ) 
          GetCachedPattern([in] int32 patternId) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCachedPattern

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetCachedParent() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCachedParent

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCachedChildren() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetCachedChildren

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentProcessId() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentProcessId

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentControlType() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentControlType

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentLocalizedControlType() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentLocalizedControlType

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentName() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentName

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentAcceleratorKey() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentAcceleratorKey

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentAccessKey() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentAccessKey

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentHasKeyboardFocus() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentHasKeyboardFocus

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsKeyboardFocusable() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentIsKeyboardFocusable

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsEnabled() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentIsEnabled

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentAutomationId() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentAutomationId

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentClassName() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentClassName

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentHelpText() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentHelpText

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentCulture() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentCulture

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsControlElement() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentIsControlElement

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsContentElement() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentIsContentElement

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsPassword() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentIsPassword

  .method public hidebysig newslot specialname abstract virtual 
          instance native int  get_CurrentNativeWindowHandle() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentNativeWindowHandle

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentItemType() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentItemType

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsOffscreen() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentIsOffscreen

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.OrientationType 
          get_CurrentOrientation() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentOrientation

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentFrameworkId() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentFrameworkId

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsRequiredForForm() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentIsRequiredForForm

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentItemStatus() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentItemStatus

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.tagRECT 
          get_CurrentBoundingRectangle() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentBoundingRectangle

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          get_CurrentLabeledBy() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentLabeledBy

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentAriaRole() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentAriaRole

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentAriaProperties() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentAriaProperties

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsDataValidForForm() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentIsDataValidForForm

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          get_CurrentControllerFor() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentControllerFor

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          get_CurrentDescribedBy() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentDescribedBy

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          get_CurrentFlowsTo() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentFlowsTo

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentProviderDescription() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CurrentProviderDescription

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedProcessId() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedProcessId

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedControlType() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedControlType

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedLocalizedControlType() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedLocalizedControlType

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedName() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedName

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedAcceleratorKey() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedAcceleratorKey

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedAccessKey() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedAccessKey

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedHasKeyboardFocus() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedHasKeyboardFocus

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsKeyboardFocusable() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedIsKeyboardFocusable

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsEnabled() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedIsEnabled

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedAutomationId() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedAutomationId

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedClassName() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedClassName

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedHelpText() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedHelpText

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedCulture() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedCulture

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsControlElement() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedIsControlElement

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsContentElement() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedIsContentElement

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsPassword() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedIsPassword

  .method public hidebysig newslot specialname abstract virtual 
          instance native int  get_CachedNativeWindowHandle() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedNativeWindowHandle

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedItemType() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedItemType

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsOffscreen() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedIsOffscreen

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.OrientationType 
          get_CachedOrientation() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedOrientation

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedFrameworkId() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedFrameworkId

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsRequiredForForm() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedIsRequiredForForm

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedItemStatus() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedItemStatus

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.tagRECT 
          get_CachedBoundingRectangle() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedBoundingRectangle

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          get_CachedLabeledBy() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedLabeledBy

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedAriaRole() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedAriaRole

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedAriaProperties() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedAriaProperties

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsDataValidForForm() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedIsDataValidForForm

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          get_CachedControllerFor() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedControllerFor

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          get_CachedDescribedBy() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedDescribedBy

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          get_CachedFlowsTo() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedFlowsTo

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedProviderDescription() runtime managed internalcall
  {
  } // end of method IUIAutomationElement::get_CachedProviderDescription

  .method public hidebysig newslot abstract virtual 
          instance int32  GetClickablePoint([out] valuetype UIAutomationClient.tagPOINT& clickable) runtime managed internalcall
  {
  } // end of method IUIAutomationElement::GetClickablePoint

  .property int32 CurrentProcessId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 11 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentProcessId()
  } // end of property IUIAutomationElement::CurrentProcessId
  .property int32 CurrentControlType()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 12 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentControlType()
  } // end of property IUIAutomationElement::CurrentControlType
  .property string CurrentLocalizedControlType()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 13 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentLocalizedControlType()
  } // end of property IUIAutomationElement::CurrentLocalizedControlType
  .property string CurrentName()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 14 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentName()
  } // end of property IUIAutomationElement::CurrentName
  .property string CurrentAcceleratorKey()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 15 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentAcceleratorKey()
  } // end of property IUIAutomationElement::CurrentAcceleratorKey
  .property string CurrentAccessKey()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 16 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentAccessKey()
  } // end of property IUIAutomationElement::CurrentAccessKey
  .property int32 CurrentHasKeyboardFocus()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 17 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentHasKeyboardFocus()
  } // end of property IUIAutomationElement::CurrentHasKeyboardFocus
  .property int32 CurrentIsKeyboardFocusable()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 18 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentIsKeyboardFocusable()
  } // end of property IUIAutomationElement::CurrentIsKeyboardFocusable
  .property int32 CurrentIsEnabled()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 19 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentIsEnabled()
  } // end of property IUIAutomationElement::CurrentIsEnabled
  .property string CurrentAutomationId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 1A 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentAutomationId()
  } // end of property IUIAutomationElement::CurrentAutomationId
  .property string CurrentClassName()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 1B 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentClassName()
  } // end of property IUIAutomationElement::CurrentClassName
  .property string CurrentHelpText()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 1C 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentHelpText()
  } // end of property IUIAutomationElement::CurrentHelpText
  .property int32 CurrentCulture()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 1D 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentCulture()
  } // end of property IUIAutomationElement::CurrentCulture
  .property int32 CurrentIsControlElement()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 1E 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentIsControlElement()
  } // end of property IUIAutomationElement::CurrentIsControlElement
  .property int32 CurrentIsContentElement()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 1F 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentIsContentElement()
  } // end of property IUIAutomationElement::CurrentIsContentElement
  .property int32 CurrentIsPassword()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 20 00 01 60 00 00 )                         // .. ..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentIsPassword()
  } // end of property IUIAutomationElement::CurrentIsPassword
  .property native int CurrentNativeWindowHandle()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 21 00 01 60 00 00 )                         // ..!..`..
    .get instance native int UIAutomationClient.IUIAutomationElement::get_CurrentNativeWindowHandle()
  } // end of property IUIAutomationElement::CurrentNativeWindowHandle
  .property string CurrentItemType()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 22 00 01 60 00 00 )                         // .."..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentItemType()
  } // end of property IUIAutomationElement::CurrentItemType
  .property int32 CurrentIsOffscreen()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 23 00 01 60 00 00 )                         // ..#..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentIsOffscreen()
  } // end of property IUIAutomationElement::CurrentIsOffscreen
  .property valuetype UIAutomationClient.OrientationType
          CurrentOrientation()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 24 00 01 60 00 00 )                         // ..$..`..
    .get instance valuetype UIAutomationClient.OrientationType UIAutomationClient.IUIAutomationElement::get_CurrentOrientation()
  } // end of property IUIAutomationElement::CurrentOrientation
  .property string CurrentFrameworkId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 25 00 01 60 00 00 )                         // ..%..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentFrameworkId()
  } // end of property IUIAutomationElement::CurrentFrameworkId
  .property int32 CurrentIsRequiredForForm()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 26 00 01 60 00 00 )                         // ..&..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentIsRequiredForForm()
  } // end of property IUIAutomationElement::CurrentIsRequiredForForm
  .property string CurrentItemStatus()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 27 00 01 60 00 00 )                         // ..'..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentItemStatus()
  } // end of property IUIAutomationElement::CurrentItemStatus
  .property valuetype UIAutomationClient.tagRECT
          CurrentBoundingRectangle()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 28 00 01 60 00 00 )                         // ..(..`..
    .get instance valuetype UIAutomationClient.tagRECT UIAutomationClient.IUIAutomationElement::get_CurrentBoundingRectangle()
  } // end of property IUIAutomationElement::CurrentBoundingRectangle
  .property class UIAutomationClient.IUIAutomationElement
          CurrentLabeledBy()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 29 00 01 60 00 00 )                         // ..)..`..
    .get instance class UIAutomationClient.IUIAutomationElement UIAutomationClient.IUIAutomationElement::get_CurrentLabeledBy()
  } // end of property IUIAutomationElement::CurrentLabeledBy
  .property string CurrentAriaRole()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 2A 00 01 60 00 00 )                         // ..*..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentAriaRole()
  } // end of property IUIAutomationElement::CurrentAriaRole
  .property string CurrentAriaProperties()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 2B 00 01 60 00 00 )                         // ..+..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentAriaProperties()
  } // end of property IUIAutomationElement::CurrentAriaProperties
  .property int32 CurrentIsDataValidForForm()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 2C 00 01 60 00 00 )                         // ..,..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CurrentIsDataValidForForm()
  } // end of property IUIAutomationElement::CurrentIsDataValidForForm
  .property class UIAutomationClient.IUIAutomationElementArray
          CurrentControllerFor()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 2D 00 01 60 00 00 )                         // ..-..`..
    .get instance class UIAutomationClient.IUIAutomationElementArray UIAutomationClient.IUIAutomationElement::get_CurrentControllerFor()
  } // end of property IUIAutomationElement::CurrentControllerFor
  .property class UIAutomationClient.IUIAutomationElementArray
          CurrentDescribedBy()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 2E 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationElementArray UIAutomationClient.IUIAutomationElement::get_CurrentDescribedBy()
  } // end of property IUIAutomationElement::CurrentDescribedBy
  .property class UIAutomationClient.IUIAutomationElementArray
          CurrentFlowsTo()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 2F 00 01 60 00 00 )                         // ../..`..
    .get instance class UIAutomationClient.IUIAutomationElementArray UIAutomationClient.IUIAutomationElement::get_CurrentFlowsTo()
  } // end of property IUIAutomationElement::CurrentFlowsTo
  .property string CurrentProviderDescription()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 30 00 01 60 00 00 )                         // ..0..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CurrentProviderDescription()
  } // end of property IUIAutomationElement::CurrentProviderDescription
  .property int32 CachedProcessId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 31 00 01 60 00 00 )                         // ..1..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedProcessId()
  } // end of property IUIAutomationElement::CachedProcessId
  .property int32 CachedControlType()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 32 00 01 60 00 00 )                         // ..2..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedControlType()
  } // end of property IUIAutomationElement::CachedControlType
  .property string CachedLocalizedControlType()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 33 00 01 60 00 00 )                         // ..3..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedLocalizedControlType()
  } // end of property IUIAutomationElement::CachedLocalizedControlType
  .property string CachedName()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 34 00 01 60 00 00 )                         // ..4..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedName()
  } // end of property IUIAutomationElement::CachedName
  .property string CachedAcceleratorKey()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 35 00 01 60 00 00 )                         // ..5..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedAcceleratorKey()
  } // end of property IUIAutomationElement::CachedAcceleratorKey
  .property string CachedAccessKey()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 36 00 01 60 00 00 )                         // ..6..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedAccessKey()
  } // end of property IUIAutomationElement::CachedAccessKey
  .property int32 CachedHasKeyboardFocus()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 37 00 01 60 00 00 )                         // ..7..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedHasKeyboardFocus()
  } // end of property IUIAutomationElement::CachedHasKeyboardFocus
  .property int32 CachedIsKeyboardFocusable()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 38 00 01 60 00 00 )                         // ..8..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedIsKeyboardFocusable()
  } // end of property IUIAutomationElement::CachedIsKeyboardFocusable
  .property int32 CachedIsEnabled()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 39 00 01 60 00 00 )                         // ..9..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedIsEnabled()
  } // end of property IUIAutomationElement::CachedIsEnabled
  .property string CachedAutomationId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 3A 00 01 60 00 00 )                         // ..:..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedAutomationId()
  } // end of property IUIAutomationElement::CachedAutomationId
  .property string CachedClassName()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 3B 00 01 60 00 00 )                         // ..;..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedClassName()
  } // end of property IUIAutomationElement::CachedClassName
  .property string CachedHelpText()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 3C 00 01 60 00 00 )                         // ..<..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedHelpText()
  } // end of property IUIAutomationElement::CachedHelpText
  .property int32 CachedCulture()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 3D 00 01 60 00 00 )                         // ..=..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedCulture()
  } // end of property IUIAutomationElement::CachedCulture
  .property int32 CachedIsControlElement()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 3E 00 01 60 00 00 )                         // ..>..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedIsControlElement()
  } // end of property IUIAutomationElement::CachedIsControlElement
  .property int32 CachedIsContentElement()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 3F 00 01 60 00 00 )                         // ..?..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedIsContentElement()
  } // end of property IUIAutomationElement::CachedIsContentElement
  .property int32 CachedIsPassword()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 40 00 01 60 00 00 )                         // ..@..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedIsPassword()
  } // end of property IUIAutomationElement::CachedIsPassword
  .property native int CachedNativeWindowHandle()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 41 00 01 60 00 00 )                         // ..A..`..
    .get instance native int UIAutomationClient.IUIAutomationElement::get_CachedNativeWindowHandle()
  } // end of property IUIAutomationElement::CachedNativeWindowHandle
  .property string CachedItemType()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 42 00 01 60 00 00 )                         // ..B..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedItemType()
  } // end of property IUIAutomationElement::CachedItemType
  .property int32 CachedIsOffscreen()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 43 00 01 60 00 00 )                         // ..C..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedIsOffscreen()
  } // end of property IUIAutomationElement::CachedIsOffscreen
  .property valuetype UIAutomationClient.OrientationType
          CachedOrientation()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 44 00 01 60 00 00 )                         // ..D..`..
    .get instance valuetype UIAutomationClient.OrientationType UIAutomationClient.IUIAutomationElement::get_CachedOrientation()
  } // end of property IUIAutomationElement::CachedOrientation
  .property string CachedFrameworkId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 45 00 01 60 00 00 )                         // ..E..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedFrameworkId()
  } // end of property IUIAutomationElement::CachedFrameworkId
  .property int32 CachedIsRequiredForForm()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 46 00 01 60 00 00 )                         // ..F..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedIsRequiredForForm()
  } // end of property IUIAutomationElement::CachedIsRequiredForForm
  .property string CachedItemStatus()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 47 00 01 60 00 00 )                         // ..G..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedItemStatus()
  } // end of property IUIAutomationElement::CachedItemStatus
  .property valuetype UIAutomationClient.tagRECT
          CachedBoundingRectangle()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 48 00 01 60 00 00 )                         // ..H..`..
    .get instance valuetype UIAutomationClient.tagRECT UIAutomationClient.IUIAutomationElement::get_CachedBoundingRectangle()
  } // end of property IUIAutomationElement::CachedBoundingRectangle
  .property class UIAutomationClient.IUIAutomationElement
          CachedLabeledBy()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 49 00 01 60 00 00 )                         // ..I..`..
    .get instance class UIAutomationClient.IUIAutomationElement UIAutomationClient.IUIAutomationElement::get_CachedLabeledBy()
  } // end of property IUIAutomationElement::CachedLabeledBy
  .property string CachedAriaRole()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 4A 00 01 60 00 00 )                         // ..J..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedAriaRole()
  } // end of property IUIAutomationElement::CachedAriaRole
  .property string CachedAriaProperties()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 4B 00 01 60 00 00 )                         // ..K..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedAriaProperties()
  } // end of property IUIAutomationElement::CachedAriaProperties
  .property int32 CachedIsDataValidForForm()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 4C 00 01 60 00 00 )                         // ..L..`..
    .get instance int32 UIAutomationClient.IUIAutomationElement::get_CachedIsDataValidForForm()
  } // end of property IUIAutomationElement::CachedIsDataValidForForm
  .property class UIAutomationClient.IUIAutomationElementArray
          CachedControllerFor()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 4D 00 01 60 00 00 )                         // ..M..`..
    .get instance class UIAutomationClient.IUIAutomationElementArray UIAutomationClient.IUIAutomationElement::get_CachedControllerFor()
  } // end of property IUIAutomationElement::CachedControllerFor
  .property class UIAutomationClient.IUIAutomationElementArray
          CachedDescribedBy()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 4E 00 01 60 00 00 )                         // ..N..`..
    .get instance class UIAutomationClient.IUIAutomationElementArray UIAutomationClient.IUIAutomationElement::get_CachedDescribedBy()
  } // end of property IUIAutomationElement::CachedDescribedBy
  .property class UIAutomationClient.IUIAutomationElementArray
          CachedFlowsTo()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 4F 00 01 60 00 00 )                         // ..O..`..
    .get instance class UIAutomationClient.IUIAutomationElementArray UIAutomationClient.IUIAutomationElement::get_CachedFlowsTo()
  } // end of property IUIAutomationElement::CachedFlowsTo
  .property string CachedProviderDescription()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 50 00 01 60 00 00 )                         // ..P..`..
    .get instance string UIAutomationClient.IUIAutomationElement::get_CachedProviderDescription()
  } // end of property IUIAutomationElement::CachedProviderDescription
} // end of class UIAutomationClient.IUIAutomationElement

.class public auto ansi sealed UIAutomationClient.TreeScope
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.TreeScope TreeScope_Element = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.TreeScope TreeScope_Children = int32(0x00000002)
  .field public static literal valuetype UIAutomationClient.TreeScope TreeScope_Descendants = int32(0x00000004)
  .field public static literal valuetype UIAutomationClient.TreeScope TreeScope_Parent = int32(0x00000008)
  .field public static literal valuetype UIAutomationClient.TreeScope TreeScope_Ancestors = int32(0x00000010)
  .field public static literal valuetype UIAutomationClient.TreeScope TreeScope_Subtree = int32(0x00000007)
} // end of class UIAutomationClient.TreeScope

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationCondition
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 33 35 32 46 46 42 41 38 2D 30 39 37 33   // ..$352FFBA8-0973
                                                                                                  2D 34 33 37 43 2D 41 36 31 46 2D 46 36 34 43 41   // -437C-A61F-F64CA
                                                                                                  46 44 38 31 44 46 39 00 00 )                      // FD81DF9..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
} // end of class UIAutomationClient.IUIAutomationCondition

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationElementArray
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 31 34 33 31 34 35 39 35 2D 42 34 42 43   // ..$14314595-B4BC
                                                                                                  2D 34 30 35 35 2D 39 35 46 32 2D 35 38 46 32 45   // -4055-95F2-58F2E
                                                                                                  34 32 43 39 38 35 35 00 00 )                      // 42C9855..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_Length() runtime managed internalcall
  {
  } // end of method IUIAutomationElementArray::get_Length

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetElement([in] int32 index) runtime managed internalcall
  {
  } // end of method IUIAutomationElementArray::GetElement

  .property int32 Length()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationElementArray::get_Length()
  } // end of property IUIAutomationElementArray::Length
} // end of class UIAutomationClient.IUIAutomationElementArray

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationCacheRequest
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 42 33 32 41 39 32 42 35 2D 42 43 32 35   // ..$B32A92B5-BC25
                                                                                                  2D 34 30 37 38 2D 39 43 30 38 2D 44 37 45 45 39   // -4078-9C08-D7EE9
                                                                                                  35 43 34 38 45 30 33 00 00 )                      // 5C48E03..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  AddProperty([in] int32 propertyId) runtime managed internalcall
  {
  } // end of method IUIAutomationCacheRequest::AddProperty

  .method public hidebysig newslot abstract virtual 
          instance void  AddPattern([in] int32 patternId) runtime managed internalcall
  {
  } // end of method IUIAutomationCacheRequest::AddPattern

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCacheRequest 
          marshal( interface ) 
          Clone() runtime managed internalcall
  {
  } // end of method IUIAutomationCacheRequest::Clone

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.TreeScope 
          get_TreeScope() runtime managed internalcall
  {
  } // end of method IUIAutomationCacheRequest::get_TreeScope

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_TreeScope([in] valuetype UIAutomationClient.TreeScope scope) runtime managed internalcall
  {
  } // end of method IUIAutomationCacheRequest::set_TreeScope

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          get_TreeFilter() runtime managed internalcall
  {
  } // end of method IUIAutomationCacheRequest::get_TreeFilter

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_TreeFilter([in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) 'filter') runtime managed internalcall
  {
  } // end of method IUIAutomationCacheRequest::set_TreeFilter

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.AutomationElementMode 
          get_AutomationElementMode() runtime managed internalcall
  {
  } // end of method IUIAutomationCacheRequest::get_AutomationElementMode

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_AutomationElementMode([in] valuetype UIAutomationClient.AutomationElementMode mode) runtime managed internalcall
  {
  } // end of method IUIAutomationCacheRequest::set_AutomationElementMode

  .property valuetype UIAutomationClient.TreeScope
          TreeScope()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.TreeScope UIAutomationClient.IUIAutomationCacheRequest::get_TreeScope()
    .set instance void UIAutomationClient.IUIAutomationCacheRequest::set_TreeScope(valuetype UIAutomationClient.TreeScope)
  } // end of property IUIAutomationCacheRequest::TreeScope
  .property class UIAutomationClient.IUIAutomationCondition
          TreeFilter()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationCondition UIAutomationClient.IUIAutomationCacheRequest::get_TreeFilter()
    .set instance void UIAutomationClient.IUIAutomationCacheRequest::set_TreeFilter(class UIAutomationClient.IUIAutomationCondition)
  } // end of property IUIAutomationCacheRequest::TreeFilter
  .property valuetype UIAutomationClient.AutomationElementMode
          AutomationElementMode()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.AutomationElementMode UIAutomationClient.IUIAutomationCacheRequest::get_AutomationElementMode()
    .set instance void UIAutomationClient.IUIAutomationCacheRequest::set_AutomationElementMode(valuetype UIAutomationClient.AutomationElementMode)
  } // end of property IUIAutomationCacheRequest::AutomationElementMode
} // end of class UIAutomationClient.IUIAutomationCacheRequest

.class public auto ansi sealed UIAutomationClient.AutomationElementMode
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.AutomationElementMode AutomationElementMode_None = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.AutomationElementMode AutomationElementMode_Full = int32(0x00000001)
} // end of class UIAutomationClient.AutomationElementMode

.class public auto ansi sealed UIAutomationClient.OrientationType
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.OrientationType OrientationType_None = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.OrientationType OrientationType_Horizontal = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.OrientationType OrientationType_Vertical = int32(0x00000002)
} // end of class UIAutomationClient.OrientationType

.class public sequential ansi sealed beforefieldinit UIAutomationClient.tagRECT
       extends [mscorlib]System.ValueType
{
  .pack 4
  .size 0
  .field public int32 left
  .field public int32 top
  .field public int32 right
  .field public int32 bottom
} // end of class UIAutomationClient.tagRECT

.class public sequential ansi sealed beforefieldinit UIAutomationClient.tagPOINT
       extends [mscorlib]System.ValueType
{
  .pack 4
  .size 0
  .field public int32 x
  .field public int32 y
} // end of class UIAutomationClient.tagPOINT

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationBoolCondition
       implements UIAutomationClient.IUIAutomationCondition
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 31 42 34 45 31 46 32 45 2D 37 35 45 42   // ..$1B4E1F2E-75EB
                                                                                                  2D 34 44 30 42 2D 38 39 35 32 2D 35 41 36 39 39   // -4D0B-8952-5A699
                                                                                                  38 38 45 32 33 30 37 00 00 )                      // 88E2307..
  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_BooleanValue() runtime managed internalcall
  {
  } // end of method IUIAutomationBoolCondition::get_BooleanValue

  .property int32 BooleanValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationBoolCondition::get_BooleanValue()
  } // end of property IUIAutomationBoolCondition::BooleanValue
} // end of class UIAutomationClient.IUIAutomationBoolCondition

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationPropertyCondition
       implements UIAutomationClient.IUIAutomationCondition
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 39 39 45 42 46 32 43 42 2D 35 35 37 38   // ..$99EBF2CB-5578
                                                                                                  2D 34 32 36 37 2D 39 41 44 34 2D 41 46 44 36 45   // -4267-9AD4-AFD6E
                                                                                                  41 37 37 45 39 34 42 00 00 )                      // A77E94B..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_propertyId() runtime managed internalcall
  {
  } // end of method IUIAutomationPropertyCondition::get_propertyId

  .method public hidebysig newslot specialname abstract virtual 
          instance object 
          marshal( struct) 
          get_PropertyValue() runtime managed internalcall
  {
  } // end of method IUIAutomationPropertyCondition::get_PropertyValue

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.PropertyConditionFlags 
          get_PropertyConditionFlags() runtime managed internalcall
  {
  } // end of method IUIAutomationPropertyCondition::get_PropertyConditionFlags

  .property int32 propertyId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationPropertyCondition::get_propertyId()
  } // end of property IUIAutomationPropertyCondition::propertyId
  .property object PropertyValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
    .get instance object UIAutomationClient.IUIAutomationPropertyCondition::get_PropertyValue()
  } // end of property IUIAutomationPropertyCondition::PropertyValue
  .property valuetype UIAutomationClient.PropertyConditionFlags
          PropertyConditionFlags()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 02 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.PropertyConditionFlags UIAutomationClient.IUIAutomationPropertyCondition::get_PropertyConditionFlags()
  } // end of property IUIAutomationPropertyCondition::PropertyConditionFlags
} // end of class UIAutomationClient.IUIAutomationPropertyCondition

.class public auto ansi sealed UIAutomationClient.PropertyConditionFlags
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.PropertyConditionFlags PropertyConditionFlags_None = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.PropertyConditionFlags PropertyConditionFlags_IgnoreCase = int32(0x00000001)
} // end of class UIAutomationClient.PropertyConditionFlags

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationAndCondition
       implements UIAutomationClient.IUIAutomationCondition
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.ComConversionLossAttribute::.ctor() = ( 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 41 37 44 30 41 46 33 36 2D 42 39 31 32   // ..$A7D0AF36-B912
                                                                                                  2D 34 35 46 45 2D 39 38 35 35 2D 30 39 31 44 44   // -45FE-9855-091DD
                                                                                                  43 31 37 34 41 45 43 00 00 )                      // C174AEC..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_ChildCount() runtime managed internalcall
  {
  } // end of method IUIAutomationAndCondition::get_ChildCount

  .method public hidebysig newslot abstract virtual 
          instance void  GetChildrenAsNativeArray([out] native int childArray,
                                                  [out] int32& childArrayCount) runtime managed internalcall
  {
  } // end of method IUIAutomationAndCondition::GetChildrenAsNativeArray

  .method public hidebysig newslot abstract virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray iunknown, "UIAutomationClient.IUIAutomationCondition") 
          GetChildren() runtime managed internalcall
  {
  } // end of method IUIAutomationAndCondition::GetChildren

  .property int32 ChildCount()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationAndCondition::get_ChildCount()
  } // end of property IUIAutomationAndCondition::ChildCount
} // end of class UIAutomationClient.IUIAutomationAndCondition

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationOrCondition
       implements UIAutomationClient.IUIAutomationCondition
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 38 37 35 33 46 30 33 32 2D 33 44 42 31   // ..$8753F032-3DB1
                                                                                                  2D 34 37 42 35 2D 41 31 46 43 2D 36 45 33 34 41   // -47B5-A1FC-6E34A
                                                                                                  32 36 36 43 37 31 32 00 00 )                      // 266C712..
  .custom instance void [mscorlib]System.Runtime.InteropServices.ComConversionLossAttribute::.ctor() = ( 01 00 00 00 ) 
  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_ChildCount() runtime managed internalcall
  {
  } // end of method IUIAutomationOrCondition::get_ChildCount

  .method public hidebysig newslot abstract virtual 
          instance void  GetChildrenAsNativeArray([out] native int childArray,
                                                  [out] int32& childArrayCount) runtime managed internalcall
  {
  } // end of method IUIAutomationOrCondition::GetChildrenAsNativeArray

  .method public hidebysig newslot abstract virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray iunknown, "UIAutomationClient.IUIAutomationCondition") 
          GetChildren() runtime managed internalcall
  {
  } // end of method IUIAutomationOrCondition::GetChildren

  .property int32 ChildCount()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationOrCondition::get_ChildCount()
  } // end of property IUIAutomationOrCondition::ChildCount
} // end of class UIAutomationClient.IUIAutomationOrCondition

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationNotCondition
       implements UIAutomationClient.IUIAutomationCondition
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 46 35 32 38 42 36 35 37 2D 38 34 37 42   // ..$F528B657-847B
                                                                                                  2D 34 39 38 43 2D 38 38 39 36 2D 44 35 32 42 35   // -498C-8896-D52B5
                                                                                                  36 35 34 30 37 41 31 00 00 )                      // 65407A1..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          GetChild() runtime managed internalcall
  {
  } // end of method IUIAutomationNotCondition::GetChild

} // end of class UIAutomationClient.IUIAutomationNotCondition

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationTreeWalker
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 34 30 34 32 43 36 32 34 2D 33 38 39 43   // ..$4042C624-389C
                                                                                                  2D 34 41 46 43 2D 41 36 33 30 2D 39 44 46 38 35   // -4AFC-A630-9DF85
                                                                                                  34 41 35 34 31 46 43 00 00 )                      // 4A541FC..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetParentElement([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetParentElement

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetFirstChildElement([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetFirstChildElement

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetLastChildElement([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetLastChildElement

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetNextSiblingElement([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetNextSiblingElement

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetPreviousSiblingElement([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetPreviousSiblingElement

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          NormalizeElement([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::NormalizeElement

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetParentElementBuildCache([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                     [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetParentElementBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetFirstChildElementBuildCache([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                         [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetFirstChildElementBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetLastChildElementBuildCache([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                        [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetLastChildElementBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetNextSiblingElementBuildCache([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                          [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetNextSiblingElementBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetPreviousSiblingElementBuildCache([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                              [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::GetPreviousSiblingElementBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          NormalizeElementBuildCache([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                     [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::NormalizeElementBuildCache

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          get_condition() runtime managed internalcall
  {
  } // end of method IUIAutomationTreeWalker::get_condition

  .property class UIAutomationClient.IUIAutomationCondition
          condition()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationCondition UIAutomationClient.IUIAutomationTreeWalker::get_condition()
  } // end of property IUIAutomationTreeWalker::condition
} // end of class UIAutomationClient.IUIAutomationTreeWalker

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationEventHandler
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 31 34 36 43 33 43 31 37 2D 46 31 32 45   // ..$146C3C17-F12E
                                                                                                  2D 34 45 32 32 2D 38 43 32 37 2D 46 38 39 34 42   // -4E22-8C27-F894B
                                                                                                  39 42 37 39 43 36 39 00 00 )                      // 9B79C69..
  .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 01 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  HandleAutomationEvent([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) sender,
                                               [in] int32 eventId) runtime managed internalcall
  {
  } // end of method IUIAutomationEventHandler::HandleAutomationEvent

} // end of class UIAutomationClient.IUIAutomationEventHandler

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationPropertyChangedEventHandler
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 34 30 43 44 33 37 44 34 2D 43 37 35 36   // ..$40CD37D4-C756
                                                                                                  2D 34 42 30 43 2D 38 43 36 46 2D 42 44 44 46 45   // -4B0C-8C6F-BDDFE
                                                                                                  45 42 31 33 42 35 30 00 00 )                      // EB13B50..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 01 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  HandlePropertyChangedEvent([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) sender,
                                                    [in] int32 propertyId,
                                                    [in] object  marshal( struct) newValue) runtime managed internalcall
  {
  } // end of method IUIAutomationPropertyChangedEventHandler::HandlePropertyChangedEvent

} // end of class UIAutomationClient.IUIAutomationPropertyChangedEventHandler

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationStructureChangedEventHandler
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 01 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 45 38 31 44 31 42 34 45 2D 31 31 43 35   // ..$E81D1B4E-11C5
                                                                                                  2D 34 32 46 38 2D 39 37 35 34 2D 45 37 30 33 36   // -42F8-9754-E7036
                                                                                                  43 37 39 46 30 35 34 00 00 )                      // C79F054..
  .method public hidebysig newslot abstract virtual 
          instance void  HandleStructureChangedEvent([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) sender,
                                                     [in] valuetype UIAutomationClient.StructureChangeType changeType,
                                                     [in] class [mscorlib]System.Array  marshal( safearray int) runtimeId) runtime managed internalcall
  {
  } // end of method IUIAutomationStructureChangedEventHandler::HandleStructureChangedEvent

} // end of class UIAutomationClient.IUIAutomationStructureChangedEventHandler

.class public auto ansi sealed UIAutomationClient.StructureChangeType
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.StructureChangeType StructureChangeType_ChildAdded = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.StructureChangeType StructureChangeType_ChildRemoved = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.StructureChangeType StructureChangeType_ChildrenInvalidated = int32(0x00000002)
  .field public static literal valuetype UIAutomationClient.StructureChangeType StructureChangeType_ChildrenBulkAdded = int32(0x00000003)
  .field public static literal valuetype UIAutomationClient.StructureChangeType StructureChangeType_ChildrenBulkRemoved = int32(0x00000004)
  .field public static literal valuetype UIAutomationClient.StructureChangeType StructureChangeType_ChildrenReordered = int32(0x00000005)
} // end of class UIAutomationClient.StructureChangeType

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationFocusChangedEventHandler
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 43 32 37 30 46 36 42 35 2D 35 43 36 39   // ..$C270F6B5-5C69
                                                                                                  2D 34 32 39 30 2D 39 37 34 35 2D 37 41 37 46 39   // -4290-9745-7A7F9
                                                                                                  37 31 36 39 34 36 38 00 00 )                      // 7169468..
  .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 01 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  HandleFocusChangedEvent([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) sender) runtime managed internalcall
  {
  } // end of method IUIAutomationFocusChangedEventHandler::HandleFocusChangedEvent

} // end of class UIAutomationClient.IUIAutomationFocusChangedEventHandler

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationInvokePattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 46 42 33 37 37 46 42 45 2D 38 45 41 36   // ..$FB377FBE-8EA6
                                                                                                  2D 34 36 44 35 2D 39 43 37 33 2D 36 34 39 39 36   // -46D5-9C73-64996
                                                                                                  34 32 44 33 30 35 39 00 00 )                      // 42D3059..
  .method public hidebysig newslot abstract virtual 
          instance void  Invoke() runtime managed internalcall
  {
  } // end of method IUIAutomationInvokePattern::Invoke

} // end of class UIAutomationClient.IUIAutomationInvokePattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationDockPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 46 44 45 35 45 46 39 37 2D 31 34 36 34   // ..$FDE5EF97-1464
                                                                                                  2D 34 38 46 36 2D 39 30 42 46 2D 34 33 44 30 39   // -48F6-90BF-43D09
                                                                                                  34 38 45 38 36 45 43 00 00 )                      // 48E86EC..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  SetDockPosition([in] valuetype UIAutomationClient.DockPosition dockPos) runtime managed internalcall
  {
  } // end of method IUIAutomationDockPattern::SetDockPosition

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.DockPosition 
          get_CurrentDockPosition() runtime managed internalcall
  {
  } // end of method IUIAutomationDockPattern::get_CurrentDockPosition

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.DockPosition 
          get_CachedDockPosition() runtime managed internalcall
  {
  } // end of method IUIAutomationDockPattern::get_CachedDockPosition

  .property valuetype UIAutomationClient.DockPosition
          CurrentDockPosition()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.DockPosition UIAutomationClient.IUIAutomationDockPattern::get_CurrentDockPosition()
  } // end of property IUIAutomationDockPattern::CurrentDockPosition
  .property valuetype UIAutomationClient.DockPosition
          CachedDockPosition()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.DockPosition UIAutomationClient.IUIAutomationDockPattern::get_CachedDockPosition()
  } // end of property IUIAutomationDockPattern::CachedDockPosition
} // end of class UIAutomationClient.IUIAutomationDockPattern

.class public auto ansi sealed UIAutomationClient.DockPosition
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.DockPosition DockPosition_Top = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.DockPosition DockPosition_Left = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.DockPosition DockPosition_Bottom = int32(0x00000002)
  .field public static literal valuetype UIAutomationClient.DockPosition DockPosition_Right = int32(0x00000003)
  .field public static literal valuetype UIAutomationClient.DockPosition DockPosition_Fill = int32(0x00000004)
  .field public static literal valuetype UIAutomationClient.DockPosition DockPosition_None = int32(0x00000005)
} // end of class UIAutomationClient.DockPosition

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationExpandCollapsePattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 36 31 39 42 45 30 38 36 2D 31 46 34 45   // ..$619BE086-1F4E
                                                                                                  2D 34 45 45 34 2D 42 41 46 41 2D 32 31 30 31 32   // -4EE4-BAFA-21012
                                                                                                  38 37 33 38 37 33 30 00 00 )                      // 8738730..
  .method public hidebysig newslot abstract virtual 
          instance void  Expand() runtime managed internalcall
  {
  } // end of method IUIAutomationExpandCollapsePattern::Expand

  .method public hidebysig newslot abstract virtual 
          instance void  Collapse() runtime managed internalcall
  {
  } // end of method IUIAutomationExpandCollapsePattern::Collapse

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.ExpandCollapseState 
          get_CurrentExpandCollapseState() runtime managed internalcall
  {
  } // end of method IUIAutomationExpandCollapsePattern::get_CurrentExpandCollapseState

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.ExpandCollapseState 
          get_CachedExpandCollapseState() runtime managed internalcall
  {
  } // end of method IUIAutomationExpandCollapsePattern::get_CachedExpandCollapseState

  .property valuetype UIAutomationClient.ExpandCollapseState
          CurrentExpandCollapseState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.ExpandCollapseState UIAutomationClient.IUIAutomationExpandCollapsePattern::get_CurrentExpandCollapseState()
  } // end of property IUIAutomationExpandCollapsePattern::CurrentExpandCollapseState
  .property valuetype UIAutomationClient.ExpandCollapseState
          CachedExpandCollapseState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.ExpandCollapseState UIAutomationClient.IUIAutomationExpandCollapsePattern::get_CachedExpandCollapseState()
  } // end of property IUIAutomationExpandCollapsePattern::CachedExpandCollapseState
} // end of class UIAutomationClient.IUIAutomationExpandCollapsePattern

.class public auto ansi sealed UIAutomationClient.ExpandCollapseState
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.ExpandCollapseState ExpandCollapseState_Collapsed = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.ExpandCollapseState ExpandCollapseState_Expanded = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.ExpandCollapseState ExpandCollapseState_PartiallyExpanded = int32(0x00000002)
  .field public static literal valuetype UIAutomationClient.ExpandCollapseState ExpandCollapseState_LeafNode = int32(0x00000003)
} // end of class UIAutomationClient.ExpandCollapseState

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationGridPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 34 31 34 43 33 43 44 43 2D 38 35 36 42   // ..$414C3CDC-856B
                                                                                                  2D 34 46 35 42 2D 38 35 33 38 2D 33 31 33 31 43   // -4F5B-8538-3131C
                                                                                                  36 33 30 32 35 35 30 00 00 )                      // 6302550..
  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetItem([in] int32 row,
                  [in] int32 column) runtime managed internalcall
  {
  } // end of method IUIAutomationGridPattern::GetItem

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentRowCount() runtime managed internalcall
  {
  } // end of method IUIAutomationGridPattern::get_CurrentRowCount

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentColumnCount() runtime managed internalcall
  {
  } // end of method IUIAutomationGridPattern::get_CurrentColumnCount

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedRowCount() runtime managed internalcall
  {
  } // end of method IUIAutomationGridPattern::get_CachedRowCount

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedColumnCount() runtime managed internalcall
  {
  } // end of method IUIAutomationGridPattern::get_CachedColumnCount

  .property int32 CurrentRowCount()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridPattern::get_CurrentRowCount()
  } // end of property IUIAutomationGridPattern::CurrentRowCount
  .property int32 CurrentColumnCount()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridPattern::get_CurrentColumnCount()
  } // end of property IUIAutomationGridPattern::CurrentColumnCount
  .property int32 CachedRowCount()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridPattern::get_CachedRowCount()
  } // end of property IUIAutomationGridPattern::CachedRowCount
  .property int32 CachedColumnCount()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridPattern::get_CachedColumnCount()
  } // end of property IUIAutomationGridPattern::CachedColumnCount
} // end of class UIAutomationClient.IUIAutomationGridPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationGridItemPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 37 38 46 38 45 46 35 37 2D 36 36 43 33   // ..$78F8EF57-66C3
                                                                                                  2D 34 45 30 39 2D 42 44 37 43 2D 45 37 39 42 32   // -4E09-BD7C-E79B2
                                                                                                  30 30 34 38 39 34 44 00 00 )                      // 004894D..
  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          get_CurrentContainingGrid() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CurrentContainingGrid

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentRow() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CurrentRow

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentColumn() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CurrentColumn

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentRowSpan() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CurrentRowSpan

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentColumnSpan() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CurrentColumnSpan

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          get_CachedContainingGrid() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CachedContainingGrid

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedRow() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CachedRow

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedColumn() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CachedColumn

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedRowSpan() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CachedRowSpan

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedColumnSpan() runtime managed internalcall
  {
  } // end of method IUIAutomationGridItemPattern::get_CachedColumnSpan

  .property class UIAutomationClient.IUIAutomationElement
          CurrentContainingGrid()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationElement UIAutomationClient.IUIAutomationGridItemPattern::get_CurrentContainingGrid()
  } // end of property IUIAutomationGridItemPattern::CurrentContainingGrid
  .property int32 CurrentRow()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridItemPattern::get_CurrentRow()
  } // end of property IUIAutomationGridItemPattern::CurrentRow
  .property int32 CurrentColumn()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridItemPattern::get_CurrentColumn()
  } // end of property IUIAutomationGridItemPattern::CurrentColumn
  .property int32 CurrentRowSpan()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridItemPattern::get_CurrentRowSpan()
  } // end of property IUIAutomationGridItemPattern::CurrentRowSpan
  .property int32 CurrentColumnSpan()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridItemPattern::get_CurrentColumnSpan()
  } // end of property IUIAutomationGridItemPattern::CurrentColumnSpan
  .property class UIAutomationClient.IUIAutomationElement
          CachedContainingGrid()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationElement UIAutomationClient.IUIAutomationGridItemPattern::get_CachedContainingGrid()
  } // end of property IUIAutomationGridItemPattern::CachedContainingGrid
  .property int32 CachedRow()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridItemPattern::get_CachedRow()
  } // end of property IUIAutomationGridItemPattern::CachedRow
  .property int32 CachedColumn()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridItemPattern::get_CachedColumn()
  } // end of property IUIAutomationGridItemPattern::CachedColumn
  .property int32 CachedRowSpan()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridItemPattern::get_CachedRowSpan()
  } // end of property IUIAutomationGridItemPattern::CachedRowSpan
  .property int32 CachedColumnSpan()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationGridItemPattern::get_CachedColumnSpan()
  } // end of property IUIAutomationGridItemPattern::CachedColumnSpan
} // end of class UIAutomationClient.IUIAutomationGridItemPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationMultipleViewPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 38 44 32 35 33 43 39 31 2D 31 44 43 35   // ..$8D253C91-1DC5
                                                                                                  2D 34 42 42 35 2D 42 31 38 46 2D 41 44 45 31 36   // -4BB5-B18F-ADE16
                                                                                                  46 41 34 39 35 45 38 00 00 )                      // FA495E8..
  .method public hidebysig newslot abstract virtual 
          instance string 
          marshal( bstr) 
          GetViewName([in] int32 view) runtime managed internalcall
  {
  } // end of method IUIAutomationMultipleViewPattern::GetViewName

  .method public hidebysig newslot abstract virtual 
          instance void  SetCurrentView([in] int32 view) runtime managed internalcall
  {
  } // end of method IUIAutomationMultipleViewPattern::SetCurrentView

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentCurrentView() runtime managed internalcall
  {
  } // end of method IUIAutomationMultipleViewPattern::get_CurrentCurrentView

  .method public hidebysig newslot abstract virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray int) 
          GetCurrentSupportedViews() runtime managed internalcall
  {
  } // end of method IUIAutomationMultipleViewPattern::GetCurrentSupportedViews

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedCurrentView() runtime managed internalcall
  {
  } // end of method IUIAutomationMultipleViewPattern::get_CachedCurrentView

  .method public hidebysig newslot abstract virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray int) 
          GetCachedSupportedViews() runtime managed internalcall
  {
  } // end of method IUIAutomationMultipleViewPattern::GetCachedSupportedViews

  .property int32 CurrentCurrentView()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationMultipleViewPattern::get_CurrentCurrentView()
  } // end of property IUIAutomationMultipleViewPattern::CurrentCurrentView
  .property int32 CachedCurrentView()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationMultipleViewPattern::get_CachedCurrentView()
  } // end of property IUIAutomationMultipleViewPattern::CachedCurrentView
} // end of class UIAutomationClient.IUIAutomationMultipleViewPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationRangeValuePattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 35 39 32 31 33 46 34 46 2D 37 33 34 36   // ..$59213F4F-7346
                                                                                                  2D 34 39 45 35 2D 42 31 32 30 2D 38 30 35 35 35   // -49E5-B120-80555
                                                                                                  39 38 37 41 31 34 38 00 00 )                      // 987A148..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  SetValue([in] float64 val) runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::SetValue

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CurrentValue() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CurrentValue

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsReadOnly() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CurrentIsReadOnly

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CurrentMaximum() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CurrentMaximum

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CurrentMinimum() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CurrentMinimum

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CurrentLargeChange() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CurrentLargeChange

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CurrentSmallChange() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CurrentSmallChange

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CachedValue() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CachedValue

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsReadOnly() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CachedIsReadOnly

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CachedMaximum() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CachedMaximum

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CachedMinimum() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CachedMinimum

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CachedLargeChange() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CachedLargeChange

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CachedSmallChange() runtime managed internalcall
  {
  } // end of method IUIAutomationRangeValuePattern::get_CachedSmallChange

  .property float64 CurrentValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CurrentValue()
  } // end of property IUIAutomationRangeValuePattern::CurrentValue
  .property int32 CurrentIsReadOnly()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationRangeValuePattern::get_CurrentIsReadOnly()
  } // end of property IUIAutomationRangeValuePattern::CurrentIsReadOnly
  .property float64 CurrentMaximum()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CurrentMaximum()
  } // end of property IUIAutomationRangeValuePattern::CurrentMaximum
  .property float64 CurrentMinimum()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CurrentMinimum()
  } // end of property IUIAutomationRangeValuePattern::CurrentMinimum
  .property float64 CurrentLargeChange()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CurrentLargeChange()
  } // end of property IUIAutomationRangeValuePattern::CurrentLargeChange
  .property float64 CurrentSmallChange()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CurrentSmallChange()
  } // end of property IUIAutomationRangeValuePattern::CurrentSmallChange
  .property float64 CachedValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CachedValue()
  } // end of property IUIAutomationRangeValuePattern::CachedValue
  .property int32 CachedIsReadOnly()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationRangeValuePattern::get_CachedIsReadOnly()
  } // end of property IUIAutomationRangeValuePattern::CachedIsReadOnly
  .property float64 CachedMaximum()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CachedMaximum()
  } // end of property IUIAutomationRangeValuePattern::CachedMaximum
  .property float64 CachedMinimum()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CachedMinimum()
  } // end of property IUIAutomationRangeValuePattern::CachedMinimum
  .property float64 CachedLargeChange()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CachedLargeChange()
  } // end of property IUIAutomationRangeValuePattern::CachedLargeChange
  .property float64 CachedSmallChange()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationRangeValuePattern::get_CachedSmallChange()
  } // end of property IUIAutomationRangeValuePattern::CachedSmallChange
} // end of class UIAutomationClient.IUIAutomationRangeValuePattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationScrollPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 38 38 46 34 44 34 32 41 2D 45 38 38 31   // ..$88F4D42A-E881
                                                                                                  2D 34 35 39 44 2D 41 37 37 43 2D 37 33 42 42 42   // -459D-A77C-73BBB
                                                                                                  42 37 45 30 32 44 43 00 00 )                      // B7E02DC..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  Scroll([in] valuetype UIAutomationClient.ScrollAmount horizontalAmount,
                                [in] valuetype UIAutomationClient.ScrollAmount verticalAmount) runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::Scroll

  .method public hidebysig newslot abstract virtual 
          instance void  SetScrollPercent([in] float64 horizontalPercent,
                                          [in] float64 verticalPercent) runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::SetScrollPercent

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CurrentHorizontalScrollPercent() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CurrentHorizontalScrollPercent

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CurrentVerticalScrollPercent() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CurrentVerticalScrollPercent

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CurrentHorizontalViewSize() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CurrentHorizontalViewSize

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CurrentVerticalViewSize() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CurrentVerticalViewSize

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentHorizontallyScrollable() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CurrentHorizontallyScrollable

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentVerticallyScrollable() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CurrentVerticallyScrollable

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CachedHorizontalScrollPercent() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CachedHorizontalScrollPercent

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CachedVerticalScrollPercent() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CachedVerticalScrollPercent

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CachedHorizontalViewSize() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CachedHorizontalViewSize

  .method public hidebysig newslot specialname abstract virtual 
          instance float64  get_CachedVerticalViewSize() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CachedVerticalViewSize

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedHorizontallyScrollable() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CachedHorizontallyScrollable

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedVerticallyScrollable() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollPattern::get_CachedVerticallyScrollable

  .property float64 CurrentHorizontalScrollPercent()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationScrollPattern::get_CurrentHorizontalScrollPercent()
  } // end of property IUIAutomationScrollPattern::CurrentHorizontalScrollPercent
  .property float64 CurrentVerticalScrollPercent()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationScrollPattern::get_CurrentVerticalScrollPercent()
  } // end of property IUIAutomationScrollPattern::CurrentVerticalScrollPercent
  .property float64 CurrentHorizontalViewSize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationScrollPattern::get_CurrentHorizontalViewSize()
  } // end of property IUIAutomationScrollPattern::CurrentHorizontalViewSize
  .property float64 CurrentVerticalViewSize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationScrollPattern::get_CurrentVerticalViewSize()
  } // end of property IUIAutomationScrollPattern::CurrentVerticalViewSize
  .property int32 CurrentHorizontallyScrollable()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationScrollPattern::get_CurrentHorizontallyScrollable()
  } // end of property IUIAutomationScrollPattern::CurrentHorizontallyScrollable
  .property int32 CurrentVerticallyScrollable()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationScrollPattern::get_CurrentVerticallyScrollable()
  } // end of property IUIAutomationScrollPattern::CurrentVerticallyScrollable
  .property float64 CachedHorizontalScrollPercent()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationScrollPattern::get_CachedHorizontalScrollPercent()
  } // end of property IUIAutomationScrollPattern::CachedHorizontalScrollPercent
  .property float64 CachedVerticalScrollPercent()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationScrollPattern::get_CachedVerticalScrollPercent()
  } // end of property IUIAutomationScrollPattern::CachedVerticalScrollPercent
  .property float64 CachedHorizontalViewSize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationScrollPattern::get_CachedHorizontalViewSize()
  } // end of property IUIAutomationScrollPattern::CachedHorizontalViewSize
  .property float64 CachedVerticalViewSize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    .get instance float64 UIAutomationClient.IUIAutomationScrollPattern::get_CachedVerticalViewSize()
  } // end of property IUIAutomationScrollPattern::CachedVerticalViewSize
  .property int32 CachedHorizontallyScrollable()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationScrollPattern::get_CachedHorizontallyScrollable()
  } // end of property IUIAutomationScrollPattern::CachedHorizontallyScrollable
  .property int32 CachedVerticallyScrollable()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationScrollPattern::get_CachedVerticallyScrollable()
  } // end of property IUIAutomationScrollPattern::CachedVerticallyScrollable
} // end of class UIAutomationClient.IUIAutomationScrollPattern

.class public auto ansi sealed UIAutomationClient.ScrollAmount
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.ScrollAmount ScrollAmount_LargeDecrement = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.ScrollAmount ScrollAmount_SmallDecrement = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.ScrollAmount ScrollAmount_NoAmount = int32(0x00000002)
  .field public static literal valuetype UIAutomationClient.ScrollAmount ScrollAmount_LargeIncrement = int32(0x00000003)
  .field public static literal valuetype UIAutomationClient.ScrollAmount ScrollAmount_SmallIncrement = int32(0x00000004)
} // end of class UIAutomationClient.ScrollAmount

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationScrollItemPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 42 34 38 38 33 30 30 46 2D 44 30 31 35   // ..$B488300F-D015
                                                                                                  2D 34 46 31 39 2D 39 43 32 39 2D 42 42 35 39 35   // -4F19-9C29-BB595
                                                                                                  45 33 36 34 35 45 46 00 00 )                      // E3645EF..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  ScrollIntoView() runtime managed internalcall
  {
  } // end of method IUIAutomationScrollItemPattern::ScrollIntoView

} // end of class UIAutomationClient.IUIAutomationScrollItemPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationSelectionPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 35 45 44 35 32 30 32 45 2D 42 32 41 43   // ..$5ED5202E-B2AC
                                                                                                  2D 34 37 41 36 2D 42 36 33 38 2D 34 42 30 42 46   // -47A6-B638-4B0BF
                                                                                                  31 34 30 44 37 38 45 00 00 )                      // 140D78E..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCurrentSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionPattern::GetCurrentSelection

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentCanSelectMultiple() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionPattern::get_CurrentCanSelectMultiple

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsSelectionRequired() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionPattern::get_CurrentIsSelectionRequired

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCachedSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionPattern::GetCachedSelection

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedCanSelectMultiple() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionPattern::get_CachedCanSelectMultiple

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsSelectionRequired() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionPattern::get_CachedIsSelectionRequired

  .property int32 CurrentCanSelectMultiple()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationSelectionPattern::get_CurrentCanSelectMultiple()
  } // end of property IUIAutomationSelectionPattern::CurrentCanSelectMultiple
  .property int32 CurrentIsSelectionRequired()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationSelectionPattern::get_CurrentIsSelectionRequired()
  } // end of property IUIAutomationSelectionPattern::CurrentIsSelectionRequired
  .property int32 CachedCanSelectMultiple()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationSelectionPattern::get_CachedCanSelectMultiple()
  } // end of property IUIAutomationSelectionPattern::CachedCanSelectMultiple
  .property int32 CachedIsSelectionRequired()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationSelectionPattern::get_CachedIsSelectionRequired()
  } // end of property IUIAutomationSelectionPattern::CachedIsSelectionRequired
} // end of class UIAutomationClient.IUIAutomationSelectionPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationSelectionItemPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 41 38 45 46 41 36 36 41 2D 30 46 44 41   // ..$A8EFA66A-0FDA
                                                                                                  2D 34 32 31 41 2D 39 31 39 34 2D 33 38 30 32 31   // -421A-9194-38021
                                                                                                  46 33 35 37 38 45 41 00 00 )                      // F3578EA..
  .method public hidebysig newslot abstract virtual 
          instance void  Select() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionItemPattern::Select

  .method public hidebysig newslot abstract virtual 
          instance void  AddToSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionItemPattern::AddToSelection

  .method public hidebysig newslot abstract virtual 
          instance void  RemoveFromSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionItemPattern::RemoveFromSelection

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsSelected() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionItemPattern::get_CurrentIsSelected

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          get_CurrentSelectionContainer() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionItemPattern::get_CurrentSelectionContainer

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsSelected() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionItemPattern::get_CachedIsSelected

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          get_CachedSelectionContainer() runtime managed internalcall
  {
  } // end of method IUIAutomationSelectionItemPattern::get_CachedSelectionContainer

  .property int32 CurrentIsSelected()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationSelectionItemPattern::get_CurrentIsSelected()
  } // end of property IUIAutomationSelectionItemPattern::CurrentIsSelected
  .property class UIAutomationClient.IUIAutomationElement
          CurrentSelectionContainer()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationElement UIAutomationClient.IUIAutomationSelectionItemPattern::get_CurrentSelectionContainer()
  } // end of property IUIAutomationSelectionItemPattern::CurrentSelectionContainer
  .property int32 CachedIsSelected()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationSelectionItemPattern::get_CachedIsSelected()
  } // end of property IUIAutomationSelectionItemPattern::CachedIsSelected
  .property class UIAutomationClient.IUIAutomationElement
          CachedSelectionContainer()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationElement UIAutomationClient.IUIAutomationSelectionItemPattern::get_CachedSelectionContainer()
  } // end of property IUIAutomationSelectionItemPattern::CachedSelectionContainer
} // end of class UIAutomationClient.IUIAutomationSelectionItemPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationSynchronizedInputPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 32 32 33 33 42 45 30 42 2D 41 46 42 37   // ..$2233BE0B-AFB7
                                                                                                  2D 34 34 38 42 2D 39 46 44 41 2D 33 42 33 37 38   // -448B-9FDA-3B378
                                                                                                  41 41 35 45 41 45 31 00 00 )                      // AA5EAE1..
  .method public hidebysig newslot abstract virtual 
          instance void  StartListening([in] valuetype UIAutomationClient.SynchronizedInputType inputType) runtime managed internalcall
  {
  } // end of method IUIAutomationSynchronizedInputPattern::StartListening

  .method public hidebysig newslot abstract virtual 
          instance void  Cancel() runtime managed internalcall
  {
  } // end of method IUIAutomationSynchronizedInputPattern::Cancel

} // end of class UIAutomationClient.IUIAutomationSynchronizedInputPattern

.class public auto ansi sealed UIAutomationClient.SynchronizedInputType
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.SynchronizedInputType SynchronizedInputType_KeyUp = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.SynchronizedInputType SynchronizedInputType_KeyDown = int32(0x00000002)
  .field public static literal valuetype UIAutomationClient.SynchronizedInputType SynchronizedInputType_LeftMouseUp = int32(0x00000004)
  .field public static literal valuetype UIAutomationClient.SynchronizedInputType SynchronizedInputType_LeftMouseDown = int32(0x00000008)
  .field public static literal valuetype UIAutomationClient.SynchronizedInputType SynchronizedInputType_RightMouseUp = int32(0x00000010)
  .field public static literal valuetype UIAutomationClient.SynchronizedInputType SynchronizedInputType_RightMouseDown = int32(0x00000020)
} // end of class UIAutomationClient.SynchronizedInputType

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationTablePattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 36 32 30 45 36 39 31 43 2D 45 41 39 36   // ..$620E691C-EA96
                                                                                                  2D 34 37 31 30 2D 41 38 35 30 2D 37 35 34 42 32   // -4710-A850-754B2
                                                                                                  34 43 45 32 34 31 37 00 00 )                      // 4CE2417..
  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCurrentRowHeaders() runtime managed internalcall
  {
  } // end of method IUIAutomationTablePattern::GetCurrentRowHeaders

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCurrentColumnHeaders() runtime managed internalcall
  {
  } // end of method IUIAutomationTablePattern::GetCurrentColumnHeaders

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.RowOrColumnMajor 
          get_CurrentRowOrColumnMajor() runtime managed internalcall
  {
  } // end of method IUIAutomationTablePattern::get_CurrentRowOrColumnMajor

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCachedRowHeaders() runtime managed internalcall
  {
  } // end of method IUIAutomationTablePattern::GetCachedRowHeaders

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCachedColumnHeaders() runtime managed internalcall
  {
  } // end of method IUIAutomationTablePattern::GetCachedColumnHeaders

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.RowOrColumnMajor 
          get_CachedRowOrColumnMajor() runtime managed internalcall
  {
  } // end of method IUIAutomationTablePattern::get_CachedRowOrColumnMajor

  .property valuetype UIAutomationClient.RowOrColumnMajor
          CurrentRowOrColumnMajor()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.RowOrColumnMajor UIAutomationClient.IUIAutomationTablePattern::get_CurrentRowOrColumnMajor()
  } // end of property IUIAutomationTablePattern::CurrentRowOrColumnMajor
  .property valuetype UIAutomationClient.RowOrColumnMajor
          CachedRowOrColumnMajor()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.RowOrColumnMajor UIAutomationClient.IUIAutomationTablePattern::get_CachedRowOrColumnMajor()
  } // end of property IUIAutomationTablePattern::CachedRowOrColumnMajor
} // end of class UIAutomationClient.IUIAutomationTablePattern

.class public auto ansi sealed UIAutomationClient.RowOrColumnMajor
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.RowOrColumnMajor RowOrColumnMajor_RowMajor = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.RowOrColumnMajor RowOrColumnMajor_ColumnMajor = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.RowOrColumnMajor RowOrColumnMajor_Indeterminate = int32(0x00000002)
} // end of class UIAutomationClient.RowOrColumnMajor

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationTableItemPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 30 42 39 36 34 45 42 33 2D 45 46 32 45   // ..$0B964EB3-EF2E
                                                                                                  2D 34 34 36 34 2D 39 43 37 39 2D 36 31 44 36 31   // -4464-9C79-61D61
                                                                                                  37 33 37 41 32 37 45 00 00 )                      // 737A27E..
  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCurrentRowHeaderItems() runtime managed internalcall
  {
  } // end of method IUIAutomationTableItemPattern::GetCurrentRowHeaderItems

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCurrentColumnHeaderItems() runtime managed internalcall
  {
  } // end of method IUIAutomationTableItemPattern::GetCurrentColumnHeaderItems

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCachedRowHeaderItems() runtime managed internalcall
  {
  } // end of method IUIAutomationTableItemPattern::GetCachedRowHeaderItems

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCachedColumnHeaderItems() runtime managed internalcall
  {
  } // end of method IUIAutomationTableItemPattern::GetCachedColumnHeaderItems

} // end of class UIAutomationClient.IUIAutomationTableItemPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationTogglePattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 39 34 43 46 38 30 35 38 2D 39 42 38 44   // ..$94CF8058-9B8D
                                                                                                  2D 34 41 42 39 2D 38 42 46 44 2D 34 43 44 30 41   // -4AB9-8BFD-4CD0A
                                                                                                  33 33 43 38 43 37 30 00 00 )                      // 33C8C70..
  .method public hidebysig newslot abstract virtual 
          instance void  Toggle() runtime managed internalcall
  {
  } // end of method IUIAutomationTogglePattern::Toggle

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.ToggleState 
          get_CurrentToggleState() runtime managed internalcall
  {
  } // end of method IUIAutomationTogglePattern::get_CurrentToggleState

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.ToggleState 
          get_CachedToggleState() runtime managed internalcall
  {
  } // end of method IUIAutomationTogglePattern::get_CachedToggleState

  .property valuetype UIAutomationClient.ToggleState
          CurrentToggleState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.ToggleState UIAutomationClient.IUIAutomationTogglePattern::get_CurrentToggleState()
  } // end of property IUIAutomationTogglePattern::CurrentToggleState
  .property valuetype UIAutomationClient.ToggleState
          CachedToggleState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.ToggleState UIAutomationClient.IUIAutomationTogglePattern::get_CachedToggleState()
  } // end of property IUIAutomationTogglePattern::CachedToggleState
} // end of class UIAutomationClient.IUIAutomationTogglePattern

.class public auto ansi sealed UIAutomationClient.ToggleState
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.ToggleState ToggleState_Off = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.ToggleState ToggleState_On = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.ToggleState ToggleState_Indeterminate = int32(0x00000002)
} // end of class UIAutomationClient.ToggleState

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationTransformPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 41 39 42 35 35 38 34 34 2D 41 35 35 44   // ..$A9B55844-A55D
                                                                                                  2D 34 45 46 30 2D 39 32 36 44 2D 35 36 39 43 31   // -4EF0-926D-569C1
                                                                                                  36 46 46 38 39 42 42 00 00 )                      // 6FF89BB..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  Move([in] float64 x,
                              [in] float64 y) runtime managed internalcall
  {
  } // end of method IUIAutomationTransformPattern::Move

  .method public hidebysig newslot abstract virtual 
          instance void  Resize([in] float64 width,
                                [in] float64 height) runtime managed internalcall
  {
  } // end of method IUIAutomationTransformPattern::Resize

  .method public hidebysig newslot abstract virtual 
          instance void  Rotate([in] float64 degrees) runtime managed internalcall
  {
  } // end of method IUIAutomationTransformPattern::Rotate

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentCanMove() runtime managed internalcall
  {
  } // end of method IUIAutomationTransformPattern::get_CurrentCanMove

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentCanResize() runtime managed internalcall
  {
  } // end of method IUIAutomationTransformPattern::get_CurrentCanResize

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentCanRotate() runtime managed internalcall
  {
  } // end of method IUIAutomationTransformPattern::get_CurrentCanRotate

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedCanMove() runtime managed internalcall
  {
  } // end of method IUIAutomationTransformPattern::get_CachedCanMove

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedCanResize() runtime managed internalcall
  {
  } // end of method IUIAutomationTransformPattern::get_CachedCanResize

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedCanRotate() runtime managed internalcall
  {
  } // end of method IUIAutomationTransformPattern::get_CachedCanRotate

  .property int32 CurrentCanMove()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationTransformPattern::get_CurrentCanMove()
  } // end of property IUIAutomationTransformPattern::CurrentCanMove
  .property int32 CurrentCanResize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationTransformPattern::get_CurrentCanResize()
  } // end of property IUIAutomationTransformPattern::CurrentCanResize
  .property int32 CurrentCanRotate()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationTransformPattern::get_CurrentCanRotate()
  } // end of property IUIAutomationTransformPattern::CurrentCanRotate
  .property int32 CachedCanMove()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationTransformPattern::get_CachedCanMove()
  } // end of property IUIAutomationTransformPattern::CachedCanMove
  .property int32 CachedCanResize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationTransformPattern::get_CachedCanResize()
  } // end of property IUIAutomationTransformPattern::CachedCanResize
  .property int32 CachedCanRotate()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationTransformPattern::get_CachedCanRotate()
  } // end of property IUIAutomationTransformPattern::CachedCanRotate
} // end of class UIAutomationClient.IUIAutomationTransformPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationValuePattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 41 39 34 43 44 38 42 31 2D 30 38 34 34   // ..$A94CD8B1-0844
                                                                                                  2D 34 43 44 36 2D 39 44 32 44 2D 36 34 30 35 33   // -4CD6-9D2D-64053
                                                                                                  37 41 42 33 39 45 39 00 00 )                      // 7AB39E9..
  .method public hidebysig newslot abstract virtual 
          instance void  SetValue([in] string  marshal( bstr) val) runtime managed internalcall
  {
  } // end of method IUIAutomationValuePattern::SetValue

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentValue() runtime managed internalcall
  {
  } // end of method IUIAutomationValuePattern::get_CurrentValue

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsReadOnly() runtime managed internalcall
  {
  } // end of method IUIAutomationValuePattern::get_CurrentIsReadOnly

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedValue() runtime managed internalcall
  {
  } // end of method IUIAutomationValuePattern::get_CachedValue

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsReadOnly() runtime managed internalcall
  {
  } // end of method IUIAutomationValuePattern::get_CachedIsReadOnly

  .property string CurrentValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationValuePattern::get_CurrentValue()
  } // end of property IUIAutomationValuePattern::CurrentValue
  .property int32 CurrentIsReadOnly()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationValuePattern::get_CurrentIsReadOnly()
  } // end of property IUIAutomationValuePattern::CurrentIsReadOnly
  .property string CachedValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationValuePattern::get_CachedValue()
  } // end of property IUIAutomationValuePattern::CachedValue
  .property int32 CachedIsReadOnly()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationValuePattern::get_CachedIsReadOnly()
  } // end of property IUIAutomationValuePattern::CachedIsReadOnly
} // end of class UIAutomationClient.IUIAutomationValuePattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationWindowPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 30 46 41 45 46 34 35 33 2D 39 32 30 38   // ..$0FAEF453-9208
                                                                                                  2D 34 33 45 46 2D 42 42 42 32 2D 33 42 34 38 35   // -43EF-BBB2-3B485
                                                                                                  31 37 37 38 36 34 46 00 00 )                      // 177864F..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  Close() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::Close

  .method public hidebysig newslot abstract virtual 
          instance int32  WaitForInputIdle([in] int32 milliseconds) runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::WaitForInputIdle

  .method public hidebysig newslot abstract virtual 
          instance void  SetWindowVisualState([in] valuetype UIAutomationClient.WindowVisualState state) runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::SetWindowVisualState

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentCanMaximize() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CurrentCanMaximize

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentCanMinimize() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CurrentCanMinimize

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsModal() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CurrentIsModal

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentIsTopmost() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CurrentIsTopmost

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.WindowVisualState 
          get_CurrentWindowVisualState() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CurrentWindowVisualState

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.WindowInteractionState 
          get_CurrentWindowInteractionState() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CurrentWindowInteractionState

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedCanMaximize() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CachedCanMaximize

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedCanMinimize() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CachedCanMinimize

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsModal() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CachedIsModal

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedIsTopmost() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CachedIsTopmost

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.WindowVisualState 
          get_CachedWindowVisualState() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CachedWindowVisualState

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.WindowInteractionState 
          get_CachedWindowInteractionState() runtime managed internalcall
  {
  } // end of method IUIAutomationWindowPattern::get_CachedWindowInteractionState

  .property int32 CurrentCanMaximize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationWindowPattern::get_CurrentCanMaximize()
  } // end of property IUIAutomationWindowPattern::CurrentCanMaximize
  .property int32 CurrentCanMinimize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationWindowPattern::get_CurrentCanMinimize()
  } // end of property IUIAutomationWindowPattern::CurrentCanMinimize
  .property int32 CurrentIsModal()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationWindowPattern::get_CurrentIsModal()
  } // end of property IUIAutomationWindowPattern::CurrentIsModal
  .property int32 CurrentIsTopmost()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationWindowPattern::get_CurrentIsTopmost()
  } // end of property IUIAutomationWindowPattern::CurrentIsTopmost
  .property valuetype UIAutomationClient.WindowVisualState
          CurrentWindowVisualState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.WindowVisualState UIAutomationClient.IUIAutomationWindowPattern::get_CurrentWindowVisualState()
  } // end of property IUIAutomationWindowPattern::CurrentWindowVisualState
  .property valuetype UIAutomationClient.WindowInteractionState
          CurrentWindowInteractionState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.WindowInteractionState UIAutomationClient.IUIAutomationWindowPattern::get_CurrentWindowInteractionState()
  } // end of property IUIAutomationWindowPattern::CurrentWindowInteractionState
  .property int32 CachedCanMaximize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationWindowPattern::get_CachedCanMaximize()
  } // end of property IUIAutomationWindowPattern::CachedCanMaximize
  .property int32 CachedCanMinimize()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationWindowPattern::get_CachedCanMinimize()
  } // end of property IUIAutomationWindowPattern::CachedCanMinimize
  .property int32 CachedIsModal()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationWindowPattern::get_CachedIsModal()
  } // end of property IUIAutomationWindowPattern::CachedIsModal
  .property int32 CachedIsTopmost()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationWindowPattern::get_CachedIsTopmost()
  } // end of property IUIAutomationWindowPattern::CachedIsTopmost
  .property valuetype UIAutomationClient.WindowVisualState
          CachedWindowVisualState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.WindowVisualState UIAutomationClient.IUIAutomationWindowPattern::get_CachedWindowVisualState()
  } // end of property IUIAutomationWindowPattern::CachedWindowVisualState
  .property valuetype UIAutomationClient.WindowInteractionState
          CachedWindowInteractionState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.WindowInteractionState UIAutomationClient.IUIAutomationWindowPattern::get_CachedWindowInteractionState()
  } // end of property IUIAutomationWindowPattern::CachedWindowInteractionState
} // end of class UIAutomationClient.IUIAutomationWindowPattern

.class public auto ansi sealed UIAutomationClient.WindowVisualState
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.WindowVisualState WindowVisualState_Normal = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.WindowVisualState WindowVisualState_Maximized = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.WindowVisualState WindowVisualState_Minimized = int32(0x00000002)
} // end of class UIAutomationClient.WindowVisualState

.class public auto ansi sealed UIAutomationClient.WindowInteractionState
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.WindowInteractionState WindowInteractionState_Running = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.WindowInteractionState WindowInteractionState_Closing = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.WindowInteractionState WindowInteractionState_ReadyForUserInteraction = int32(0x00000002)
  .field public static literal valuetype UIAutomationClient.WindowInteractionState WindowInteractionState_BlockedByModalWindow = int32(0x00000003)
  .field public static literal valuetype UIAutomationClient.WindowInteractionState WindowInteractionState_NotResponding = int32(0x00000004)
} // end of class UIAutomationClient.WindowInteractionState

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationTextRange
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 41 35 34 33 43 43 36 41 2D 46 34 41 45   // ..$A543CC6A-F4AE
                                                                                                  2D 34 39 34 42 2D 38 32 33 39 2D 43 38 31 34 34   // -494B-8239-C8144
                                                                                                  38 31 31 38 37 41 38 00 00 )                      // 81187A8..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationTextRange 
          marshal( interface ) 
          Clone() runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::Clone

  .method public hidebysig newslot abstract virtual 
          instance int32  Compare([in] class UIAutomationClient.IUIAutomationTextRange  marshal( interface ) range) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::Compare

  .method public hidebysig newslot abstract virtual 
          instance int32  CompareEndpoints([in] valuetype UIAutomationClient.TextPatternRangeEndpoint srcEndPoint,
                                           [in] class UIAutomationClient.IUIAutomationTextRange  marshal( interface ) range,
                                           [in] valuetype UIAutomationClient.TextPatternRangeEndpoint targetEndPoint) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::CompareEndpoints

  .method public hidebysig newslot abstract virtual 
          instance void  ExpandToEnclosingUnit([in] valuetype UIAutomationClient.TextUnit TextUnit) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::ExpandToEnclosingUnit

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationTextRange 
          marshal( interface ) 
          FindAttribute([in] int32 attr,
                        [in] object  marshal( struct) val,
                        [in] int32 backward) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::FindAttribute

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationTextRange 
          marshal( interface ) 
          FindText([in] string  marshal( bstr) text,
                   [in] int32 backward,
                   [in] int32 ignoreCase) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::FindText

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( struct) 
          GetAttributeValue([in] int32 attr) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::GetAttributeValue

  .method public hidebysig newslot abstract virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray float64) 
          GetBoundingRectangles() runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::GetBoundingRectangles

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetEnclosingElement() runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::GetEnclosingElement

  .method public hidebysig newslot abstract virtual 
          instance string 
          marshal( bstr) 
          GetText([in] int32 maxLength) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::GetText

  .method public hidebysig newslot abstract virtual 
          instance int32  Move([in] valuetype UIAutomationClient.TextUnit unit,
                               [in] int32 count) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::Move

  .method public hidebysig newslot abstract virtual 
          instance int32  MoveEndpointByUnit([in] valuetype UIAutomationClient.TextPatternRangeEndpoint endpoint,
                                             [in] valuetype UIAutomationClient.TextUnit unit,
                                             [in] int32 count) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::MoveEndpointByUnit

  .method public hidebysig newslot abstract virtual 
          instance void  MoveEndpointByRange([in] valuetype UIAutomationClient.TextPatternRangeEndpoint srcEndPoint,
                                             [in] class UIAutomationClient.IUIAutomationTextRange  marshal( interface ) range,
                                             [in] valuetype UIAutomationClient.TextPatternRangeEndpoint targetEndPoint) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::MoveEndpointByRange

  .method public hidebysig newslot abstract virtual 
          instance void  Select() runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::Select

  .method public hidebysig newslot abstract virtual 
          instance void  AddToSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::AddToSelection

  .method public hidebysig newslot abstract virtual 
          instance void  RemoveFromSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::RemoveFromSelection

  .method public hidebysig newslot abstract virtual 
          instance void  ScrollIntoView([in] int32 alignToTop) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::ScrollIntoView

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetChildren() runtime managed internalcall
  {
  } // end of method IUIAutomationTextRange::GetChildren

} // end of class UIAutomationClient.IUIAutomationTextRange

.class public auto ansi sealed UIAutomationClient.TextPatternRangeEndpoint
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.TextPatternRangeEndpoint TextPatternRangeEndpoint_Start = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.TextPatternRangeEndpoint TextPatternRangeEndpoint_End = int32(0x00000001)
} // end of class UIAutomationClient.TextPatternRangeEndpoint

.class public auto ansi sealed UIAutomationClient.TextUnit
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.TextUnit TextUnit_Character = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.TextUnit TextUnit_Format = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.TextUnit TextUnit_Word = int32(0x00000002)
  .field public static literal valuetype UIAutomationClient.TextUnit TextUnit_Line = int32(0x00000003)
  .field public static literal valuetype UIAutomationClient.TextUnit TextUnit_Paragraph = int32(0x00000004)
  .field public static literal valuetype UIAutomationClient.TextUnit TextUnit_Page = int32(0x00000005)
  .field public static literal valuetype UIAutomationClient.TextUnit TextUnit_Document = int32(0x00000006)
} // end of class UIAutomationClient.TextUnit

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationTextRangeArray
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 43 45 34 41 45 37 36 41 2D 45 37 31 37   // ..$CE4AE76A-E717
                                                                                                  2D 34 43 39 38 2D 38 31 45 41 2D 34 37 33 37 31   // -4C98-81EA-47371
                                                                                                  44 30 32 38 45 42 36 00 00 )                      // D028EB6..
  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_Length() runtime managed internalcall
  {
  } // end of method IUIAutomationTextRangeArray::get_Length

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationTextRange 
          marshal( interface ) 
          GetElement([in] int32 index) runtime managed internalcall
  {
  } // end of method IUIAutomationTextRangeArray::GetElement

  .property int32 Length()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationTextRangeArray::get_Length()
  } // end of property IUIAutomationTextRangeArray::Length
} // end of class UIAutomationClient.IUIAutomationTextRangeArray

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationTextPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 33 32 45 42 41 32 38 39 2D 33 35 38 33   // ..$32EBA289-3583
                                                                                                  2D 34 32 43 39 2D 39 43 35 39 2D 33 42 36 44 39   // -42C9-9C59-3B6D9
                                                                                                  41 31 45 39 42 36 41 00 00 )                      // A1E9B6A..
  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationTextRange 
          marshal( interface ) 
          RangeFromPoint([in] valuetype UIAutomationClient.tagPOINT pt) runtime managed internalcall
  {
  } // end of method IUIAutomationTextPattern::RangeFromPoint

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationTextRange 
          marshal( interface ) 
          RangeFromChild([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) child) runtime managed internalcall
  {
  } // end of method IUIAutomationTextPattern::RangeFromChild

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationTextRangeArray 
          marshal( interface ) 
          GetSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationTextPattern::GetSelection

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationTextRangeArray 
          marshal( interface ) 
          GetVisibleRanges() runtime managed internalcall
  {
  } // end of method IUIAutomationTextPattern::GetVisibleRanges

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationTextRange 
          marshal( interface ) 
          get_DocumentRange() runtime managed internalcall
  {
  } // end of method IUIAutomationTextPattern::get_DocumentRange

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.SupportedTextSelection 
          get_SupportedTextSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationTextPattern::get_SupportedTextSelection

  .property class UIAutomationClient.IUIAutomationTextRange
          DocumentRange()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationTextRange UIAutomationClient.IUIAutomationTextPattern::get_DocumentRange()
  } // end of property IUIAutomationTextPattern::DocumentRange
  .property valuetype UIAutomationClient.SupportedTextSelection
          SupportedTextSelection()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.SupportedTextSelection UIAutomationClient.IUIAutomationTextPattern::get_SupportedTextSelection()
  } // end of property IUIAutomationTextPattern::SupportedTextSelection
} // end of class UIAutomationClient.IUIAutomationTextPattern

.class public auto ansi sealed UIAutomationClient.SupportedTextSelection
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.SupportedTextSelection SupportedTextSelection_None = int32(0x00000000)
  .field public static literal valuetype UIAutomationClient.SupportedTextSelection SupportedTextSelection_Single = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.SupportedTextSelection SupportedTextSelection_Multiple = int32(0x00000002)
} // end of class UIAutomationClient.SupportedTextSelection

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationLegacyIAccessiblePattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 38 32 38 30 35 35 41 44 2D 33 35 35 42   // ..$828055AD-355B
                                                                                                  2D 34 34 33 35 2D 38 36 44 35 2D 33 42 35 31 43   // -4435-86D5-3B51C
                                                                                                  31 34 41 39 42 31 42 00 00 )                      // 14A9B1B..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance void  Select(int32 flagsSelect) runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::Select

  .method public hidebysig newslot abstract virtual 
          instance void  DoDefaultAction() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::DoDefaultAction

  .method public hidebysig newslot abstract virtual 
          instance void  SetValue(string  marshal( lpwstr) szValue) runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::SetValue

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CurrentChildId() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CurrentChildId

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentName() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CurrentName

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentValue() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CurrentValue

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentDescription() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CurrentDescription

  .method public hidebysig newslot specialname abstract virtual 
          instance uint32  get_CurrentRole() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CurrentRole

  .method public hidebysig newslot specialname abstract virtual 
          instance uint32  get_CurrentState() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CurrentState

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentHelp() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CurrentHelp

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentKeyboardShortcut() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CurrentKeyboardShortcut

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCurrentSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::GetCurrentSelection

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CurrentDefaultAction() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CurrentDefaultAction

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CachedChildId() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CachedChildId

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedName() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CachedName

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedValue() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CachedValue

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedDescription() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CachedDescription

  .method public hidebysig newslot specialname abstract virtual 
          instance uint32  get_CachedRole() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CachedRole

  .method public hidebysig newslot specialname abstract virtual 
          instance uint32  get_CachedState() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CachedState

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedHelp() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CachedHelp

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedKeyboardShortcut() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CachedKeyboardShortcut

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElementArray 
          marshal( interface ) 
          GetCachedSelection() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::GetCachedSelection

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_CachedDefaultAction() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::get_CachedDefaultAction

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IAccessible 
          marshal( interface ) 
          GetIAccessible() runtime managed internalcall
  {
  } // end of method IUIAutomationLegacyIAccessiblePattern::GetIAccessible

  .property int32 CurrentChildId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CurrentChildId()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CurrentChildId
  .property string CurrentName()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CurrentName()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CurrentName
  .property string CurrentValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CurrentValue()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CurrentValue
  .property string CurrentDescription()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CurrentDescription()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CurrentDescription
  .property uint32 CurrentRole()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    .get instance uint32 UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CurrentRole()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CurrentRole
  .property uint32 CurrentState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    .get instance uint32 UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CurrentState()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CurrentState
  .property string CurrentHelp()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CurrentHelp()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CurrentHelp
  .property string CurrentKeyboardShortcut()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CurrentKeyboardShortcut()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CurrentKeyboardShortcut
  .property string CurrentDefaultAction()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CurrentDefaultAction()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CurrentDefaultAction
  .property int32 CachedChildId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CachedChildId()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CachedChildId
  .property string CachedName()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CachedName()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CachedName
  .property string CachedValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CachedValue()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CachedValue
  .property string CachedDescription()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 10 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CachedDescription()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CachedDescription
  .property uint32 CachedRole()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 11 00 01 60 00 00 )                         // .....`..
    .get instance uint32 UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CachedRole()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CachedRole
  .property uint32 CachedState()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 12 00 01 60 00 00 )                         // .....`..
    .get instance uint32 UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CachedState()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CachedState
  .property string CachedHelp()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 13 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CachedHelp()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CachedHelp
  .property string CachedKeyboardShortcut()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 14 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CachedKeyboardShortcut()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CachedKeyboardShortcut
  .property string CachedDefaultAction()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 16 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationLegacyIAccessiblePattern::get_CachedDefaultAction()
  } // end of property IUIAutomationLegacyIAccessiblePattern::CachedDefaultAction
} // end of class UIAutomationClient.IUIAutomationLegacyIAccessiblePattern

.class interface public abstract auto ansi import UIAutomationClient.IAccessible
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 50 10 00 00 )                               // ..P...
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 36 31 38 37 33 36 45 30 2D 33 43 33 44   // ..$618736E0-3C3D
                                                                                                  2D 31 31 43 46 2D 38 31 30 43 2D 30 30 41 41 30   // -11CF-810C-00AA0
                                                                                                  30 33 38 39 42 37 31 00 00 )                      // 0389B71..
  .method public hidebysig newslot specialname abstract virtual 
          instance object 
          marshal( idispatch ) 
          get_accParent() runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 78 EC FF FF 00 00 )                         // ..x.....
  } // end of method IAccessible::get_accParent

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_accChildCount() runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 77 EC FF FF 00 00 )                         // ..w.....
  } // end of method IAccessible::get_accChildCount

  .method public hidebysig newslot specialname abstract virtual 
          instance object 
          marshal( idispatch ) 
          get_accChild([in] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 76 EC FF FF 00 00 )                         // ..v.....
  } // end of method IAccessible::get_accChild

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_accName([in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 75 EC FF FF 00 00 )                         // ..u.....
  } // end of method IAccessible::get_accName

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_accValue([in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 74 EC FF FF 00 00 )                         // ..t.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::get_accValue

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_accDescription([in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 73 EC FF FF 00 00 )                         // ..s.....
  } // end of method IAccessible::get_accDescription

  .method public hidebysig newslot specialname abstract virtual 
          instance object 
          marshal( struct) 
          get_accRole([in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 72 EC FF FF 00 00 )                         // ..r.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::get_accRole

  .method public hidebysig newslot specialname abstract virtual 
          instance object 
          marshal( struct) 
          get_accState([in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 71 EC FF FF 00 00 )                         // ..q.....
  } // end of method IAccessible::get_accState

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_accHelp([in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 70 EC FF FF 00 00 )                         // ..p.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::get_accHelp

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_accHelpTopic([out] string&  marshal( bstr) pszHelpFile,
                                           [in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6F EC FF FF 00 00 )                         // ..o.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::get_accHelpTopic

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_accKeyboardShortcut([in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6E EC FF FF 00 00 )                         // ..n.....
  } // end of method IAccessible::get_accKeyboardShortcut

  .method public hidebysig newslot specialname abstract virtual 
          instance object 
          marshal( struct) 
          get_accFocus() runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6D EC FF FF 00 00 )                         // ..m.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::get_accFocus

  .method public hidebysig newslot specialname abstract virtual 
          instance object 
          marshal( struct) 
          get_accSelection() runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6C EC FF FF 00 00 )                         // ..l.....
  } // end of method IAccessible::get_accSelection

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_accDefaultAction([in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6B EC FF FF 00 00 )                         // ..k.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::get_accDefaultAction

  .method public hidebysig newslot abstract virtual 
          instance void  accSelect([in] int32 flagsSelect,
                                   [in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6A EC FF FF 00 00 )                         // ..j.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::accSelect

  .method public hidebysig newslot abstract virtual 
          instance void  accLocation([out] int32& pxLeft,
                                     [out] int32& pyTop,
                                     [out] int32& pcxWidth,
                                     [out] int32& pcyHeight,
                                     [in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 69 EC FF FF 00 00 )                         // ..i.....
  } // end of method IAccessible::accLocation

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( struct) 
          accNavigate([in] int32 navDir,
                      [in][opt] object  marshal( struct) varStart) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 68 EC FF FF 00 00 )                         // ..h.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::accNavigate

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( struct) 
          accHitTest([in] int32 xLeft,
                     [in] int32 yTop) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 67 EC FF FF 00 00 )                         // ..g.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::accHitTest

  .method public hidebysig newslot abstract virtual 
          instance void  accDoDefaultAction([in][opt] object  marshal( struct) varChild) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 66 EC FF FF 00 00 )                         // ..f.....
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
  } // end of method IAccessible::accDoDefaultAction

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_accName([in][opt] object  marshal( struct) varChild,
                                     [in] string  marshal( bstr) pszName) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 75 EC FF FF 00 00 )                         // ..u.....
  } // end of method IAccessible::set_accName

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_accValue([in][opt] object  marshal( struct) varChild,
                                      [in] string  marshal( bstr) pszValue) runtime managed internalcall
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibFuncAttribute::.ctor(int16) = ( 01 00 40 00 00 00 )                               // ..@...
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 74 EC FF FF 00 00 )                         // ..t.....
  } // end of method IAccessible::set_accValue

  .property object accParent()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 78 EC FF FF 00 00 )                         // ..x.....
    .get instance object UIAutomationClient.IAccessible::get_accParent()
  } // end of property IAccessible::accParent
  .property int32 accChildCount()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 77 EC FF FF 00 00 )                         // ..w.....
    .get instance int32 UIAutomationClient.IAccessible::get_accChildCount()
  } // end of property IAccessible::accChildCount
  .property object accChild(object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 76 EC FF FF 00 00 )                         // ..v.....
    .get instance object UIAutomationClient.IAccessible::get_accChild(object)
  } // end of property IAccessible::accChild
  .property string accName(object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 75 EC FF FF 00 00 )                         // ..u.....
    .get instance string UIAutomationClient.IAccessible::get_accName(object)
    .set instance void UIAutomationClient.IAccessible::set_accName(object,
                                                                   string)
  } // end of property IAccessible::accName
  .property string accValue(object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 74 EC FF FF 00 00 )                         // ..t.....
    .set instance void UIAutomationClient.IAccessible::set_accValue(object,
                                                                    string)
    .get instance string UIAutomationClient.IAccessible::get_accValue(object)
  } // end of property IAccessible::accValue
  .property string accDescription(object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 73 EC FF FF 00 00 )                         // ..s.....
    .get instance string UIAutomationClient.IAccessible::get_accDescription(object)
  } // end of property IAccessible::accDescription
  .property object accRole(object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 72 EC FF FF 00 00 )                         // ..r.....
    .get instance object UIAutomationClient.IAccessible::get_accRole(object)
  } // end of property IAccessible::accRole
  .property object accState(object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 71 EC FF FF 00 00 )                         // ..q.....
    .get instance object UIAutomationClient.IAccessible::get_accState(object)
  } // end of property IAccessible::accState
  .property string accHelp(object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 70 EC FF FF 00 00 )                         // ..p.....
    .get instance string UIAutomationClient.IAccessible::get_accHelp(object)
  } // end of property IAccessible::accHelp
  .property int32 accHelpTopic(string&,
                               object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6F EC FF FF 00 00 )                         // ..o.....
    .get instance int32 UIAutomationClient.IAccessible::get_accHelpTopic(string&,
                                                                         object)
  } // end of property IAccessible::accHelpTopic
  .property string accKeyboardShortcut(object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6E EC FF FF 00 00 )                         // ..n.....
    .get instance string UIAutomationClient.IAccessible::get_accKeyboardShortcut(object)
  } // end of property IAccessible::accKeyboardShortcut
  .property object accFocus()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6D EC FF FF 00 00 )                         // ..m.....
    .get instance object UIAutomationClient.IAccessible::get_accFocus()
  } // end of property IAccessible::accFocus
  .property object accSelection()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6C EC FF FF 00 00 )                         // ..l.....
    .get instance object UIAutomationClient.IAccessible::get_accSelection()
  } // end of property IAccessible::accSelection
  .property string accDefaultAction(object)
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 6B EC FF FF 00 00 )                         // ..k.....
    .get instance string UIAutomationClient.IAccessible::get_accDefaultAction(object)
  } // end of property IAccessible::accDefaultAction
} // end of class UIAutomationClient.IAccessible

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationItemContainerPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 43 36 39 30 46 44 42 32 2D 32 37 41 38   // ..$C690FDB2-27A8
                                                                                                  2D 34 32 33 43 2D 38 31 32 44 2D 34 32 39 37 37   // -423C-812D-42977
                                                                                                  33 43 39 30 38 34 45 00 00 )                      // 3C9084E..
  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          FindItemByProperty([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) pStartAfter,
                             [in] int32 propertyId,
                             [in] object  marshal( struct) 'value') runtime managed internalcall
  {
  } // end of method IUIAutomationItemContainerPattern::FindItemByProperty

} // end of class UIAutomationClient.IUIAutomationItemContainerPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationVirtualizedItemPattern
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 36 42 41 33 44 37 41 36 2D 30 34 43 46   // ..$6BA3D7A6-04CF
                                                                                                  2D 34 46 31 31 2D 38 37 39 33 2D 41 38 44 31 43   // -4F11-8793-A8D1C
                                                                                                  44 45 39 39 36 39 46 00 00 )                      // DE9969F..
  .method public hidebysig newslot abstract virtual 
          instance void  Realize() runtime managed internalcall
  {
  } // end of method IUIAutomationVirtualizedItemPattern::Realize

} // end of class UIAutomationClient.IUIAutomationVirtualizedItemPattern

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationProxyFactory
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 38 35 42 39 34 45 43 44 2D 38 34 39 44   // ..$85B94ECD-849D
                                                                                                  2D 34 32 42 36 2D 42 39 34 44 2D 44 36 44 42 32   // -42B6-B94D-D6DB2
                                                                                                  33 46 44 46 35 41 34 00 00 )                      // 3FDF5A4..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance class System.Windows.Automation.Providers.IRawElementProviderSimple 
          marshal( interface ) 
          CreateProvider([in] native int hwnd,
                         [in] int32 idObject,
                         [in] int32 idChild) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactory::CreateProvider

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_ProxyFactoryId() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactory::get_ProxyFactoryId

  .property string ProxyFactoryId()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationProxyFactory::get_ProxyFactoryId()
  } // end of property IUIAutomationProxyFactory::ProxyFactoryId
} // end of class UIAutomationClient.IUIAutomationProxyFactory

.class interface public abstract auto ansi import System.Windows.Automation.Providers.IRawElementProviderSimple
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 44 36 44 44 36 38 44 31 2D 38 36 46 44   // ..$D6DD68D1-86FD
                                                                                                  2D 34 33 33 32 2D 38 36 36 36 2D 39 41 42 45 44   // -4332-8666-9ABED
                                                                                                  45 41 32 44 32 34 43 00 00 )                      // EA2D24C..
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype UIAutomationClient.ProviderOptions 
          get_ProviderOptions() runtime managed internalcall
  {
  } // end of method IRawElementProviderSimple::get_ProviderOptions

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( iunknown ) 
          GetPatternProvider([in] int32 patternId) runtime managed internalcall
  {
  } // end of method IRawElementProviderSimple::GetPatternProvider

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( struct) 
          GetPropertyValue([in] int32 propertyId) runtime managed internalcall
  {
  } // end of method IRawElementProviderSimple::GetPropertyValue

  .method public hidebysig newslot specialname abstract virtual 
          instance class System.Windows.Automation.Providers.IRawElementProviderSimple 
          marshal( interface ) 
          get_HostRawElementProvider() runtime managed internalcall
  {
  } // end of method IRawElementProviderSimple::get_HostRawElementProvider

  .property valuetype UIAutomationClient.ProviderOptions
          ProviderOptions()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    .get instance valuetype UIAutomationClient.ProviderOptions System.Windows.Automation.Providers.IRawElementProviderSimple::get_ProviderOptions()
  } // end of property IRawElementProviderSimple::ProviderOptions
  .property class System.Windows.Automation.Providers.IRawElementProviderSimple
          HostRawElementProvider()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance class System.Windows.Automation.Providers.IRawElementProviderSimple System.Windows.Automation.Providers.IRawElementProviderSimple::get_HostRawElementProvider()
  } // end of property IRawElementProviderSimple::HostRawElementProvider
} // end of class System.Windows.Automation.Providers.IRawElementProviderSimple

.class public auto ansi sealed UIAutomationClient.ProviderOptions
       extends [mscorlib]System.Enum
{
  .field public specialname rtspecialname int32 value__
  .field public static literal valuetype UIAutomationClient.ProviderOptions ProviderOptions_ClientSideProvider = int32(0x00000001)
  .field public static literal valuetype UIAutomationClient.ProviderOptions ProviderOptions_ServerSideProvider = int32(0x00000002)
  .field public static literal valuetype UIAutomationClient.ProviderOptions ProviderOptions_NonClientAreaProvider = int32(0x00000004)
  .field public static literal valuetype UIAutomationClient.ProviderOptions ProviderOptions_OverrideProvider = int32(0x00000008)
  .field public static literal valuetype UIAutomationClient.ProviderOptions ProviderOptions_ProviderOwnsSetFocus = int32(0x00000010)
  .field public static literal valuetype UIAutomationClient.ProviderOptions ProviderOptions_UseComThreading = int32(0x00000020)
} // end of class UIAutomationClient.ProviderOptions

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationProxyFactoryEntry
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 44 35 30 45 34 37 32 45 2D 42 36 34 42   // ..$D50E472E-B64B
                                                                                                  2D 34 39 30 43 2D 42 43 41 31 2D 44 33 30 36 39   // -490C-BCA1-D3069
                                                                                                  36 46 39 46 32 38 39 00 00 )                      // 6F9F289..
  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationProxyFactory 
          marshal( interface ) 
          get_ProxyFactory() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::get_ProxyFactory

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_ClassName() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::get_ClassName

  .method public hidebysig newslot specialname abstract virtual 
          instance string 
          marshal( bstr) 
          get_ImageName() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::get_ImageName

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_AllowSubstringMatch() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::get_AllowSubstringMatch

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_CanCheckBaseClass() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::get_CanCheckBaseClass

  .method public hidebysig newslot specialname abstract virtual 
          instance int32  get_NeedsAdviseEvents() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::get_NeedsAdviseEvents

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_ClassName([in] string  marshal( lpwstr) ClassName) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::set_ClassName

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_ImageName([in] string  marshal( lpwstr) ImageName) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::set_ImageName

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_AllowSubstringMatch([in] int32 AllowSubstringMatch) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::set_AllowSubstringMatch

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_CanCheckBaseClass([in] int32 CanCheckBaseClass) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::set_CanCheckBaseClass

  .method public hidebysig newslot specialname abstract virtual 
          instance void  set_NeedsAdviseEvents([in] int32 adviseEvents) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::set_NeedsAdviseEvents

  .method public hidebysig newslot abstract virtual 
          instance void  SetWinEventsForAutomationEvent([in] int32 eventId,
                                                        [in] int32 propertyId,
                                                        [in] class [mscorlib]System.Array  marshal( safearray unsigned int) winEvents) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::SetWinEventsForAutomationEvent

  .method public hidebysig newslot abstract virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray unsigned int) 
          GetWinEventsForAutomationEvent([in] int32 eventId,
                                         [in] int32 propertyId) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryEntry::GetWinEventsForAutomationEvent

  .property class UIAutomationClient.IUIAutomationProxyFactory
          ProxyFactory()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationProxyFactory UIAutomationClient.IUIAutomationProxyFactoryEntry::get_ProxyFactory()
  } // end of property IUIAutomationProxyFactoryEntry::ProxyFactory
  .property string ClassName()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    .get instance string UIAutomationClient.IUIAutomationProxyFactoryEntry::get_ClassName()
    .set instance void UIAutomationClient.IUIAutomationProxyFactoryEntry::set_ClassName(string)
  } // end of property IUIAutomationProxyFactoryEntry::ClassName
  .property string ImageName()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    .set instance void UIAutomationClient.IUIAutomationProxyFactoryEntry::set_ImageName(string)
    .get instance string UIAutomationClient.IUIAutomationProxyFactoryEntry::get_ImageName()
  } // end of property IUIAutomationProxyFactoryEntry::ImageName
  .property int32 AllowSubstringMatch()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationProxyFactoryEntry::get_AllowSubstringMatch()
    .set instance void UIAutomationClient.IUIAutomationProxyFactoryEntry::set_AllowSubstringMatch(int32)
  } // end of property IUIAutomationProxyFactoryEntry::AllowSubstringMatch
  .property int32 CanCheckBaseClass()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    .set instance void UIAutomationClient.IUIAutomationProxyFactoryEntry::set_CanCheckBaseClass(int32)
    .get instance int32 UIAutomationClient.IUIAutomationProxyFactoryEntry::get_CanCheckBaseClass()
  } // end of property IUIAutomationProxyFactoryEntry::CanCheckBaseClass
  .property int32 NeedsAdviseEvents()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    .get instance int32 UIAutomationClient.IUIAutomationProxyFactoryEntry::get_NeedsAdviseEvents()
    .set instance void UIAutomationClient.IUIAutomationProxyFactoryEntry::set_NeedsAdviseEvents(int32)
  } // end of property IUIAutomationProxyFactoryEntry::NeedsAdviseEvents
} // end of class UIAutomationClient.IUIAutomationProxyFactoryEntry

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomationProxyFactoryMapping
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 30 39 45 33 31 45 31 38 2D 38 37 32 44   // ..$09E31E18-872D
                                                                                                  2D 34 38 37 33 2D 39 33 44 31 2D 31 45 35 34 31   // -4873-93D1-1E541
                                                                                                  45 43 31 33 33 46 44 00 00 )                      // EC133FD..
  .method public hidebysig newslot specialname abstract virtual 
          instance uint32  get_count() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryMapping::get_count

  .method public hidebysig newslot abstract virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray iunknown, "UIAutomationClient.IUIAutomationProxyFactoryEntry") 
          GetTable() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryMapping::GetTable

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationProxyFactoryEntry 
          marshal( interface ) 
          GetEntry([in] uint32 index) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryMapping::GetEntry

  .method public hidebysig newslot abstract virtual 
          instance void  SetTable([in] class [mscorlib]System.Array  marshal( safearray iunknown,
                                   "UIAutomationClient.IUIAutomationProxyFactoryEntry") factoryList) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryMapping::SetTable

  .method public hidebysig newslot abstract virtual 
          instance void  InsertEntries([in] uint32 before,
                                       [in] class [mscorlib]System.Array  marshal( safearray iunknown,
                                        "UIAutomationClient.IUIAutomationProxyFactoryEntry") factoryList) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryMapping::InsertEntries

  .method public hidebysig newslot abstract virtual 
          instance void  InsertEntry([in] uint32 before,
                                     [in] class UIAutomationClient.IUIAutomationProxyFactoryEntry  marshal( interface ) factory) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryMapping::InsertEntry

  .method public hidebysig newslot abstract virtual 
          instance void  RemoveEntry([in] uint32 index) runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryMapping::RemoveEntry

  .method public hidebysig newslot abstract virtual 
          instance void  ClearTable() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryMapping::ClearTable

  .method public hidebysig newslot abstract virtual 
          instance void  RestoreDefaultTable() runtime managed internalcall
  {
  } // end of method IUIAutomationProxyFactoryMapping::RestoreDefaultTable

  .property uint32 count()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    .get instance uint32 UIAutomationClient.IUIAutomationProxyFactoryMapping::get_count()
  } // end of property IUIAutomationProxyFactoryMapping::count
} // end of class UIAutomationClient.IUIAutomationProxyFactoryMapping

.class interface public abstract auto ansi import UIAutomationClient.IUIAutomation
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 33 30 43 42 45 35 37 44 2D 44 39 44 30   // ..$30CBE57D-D9D0
                                                                                                  2D 34 35 32 41 2D 41 42 31 33 2D 37 41 43 35 41   // -452A-AB13-7AC5A
                                                                                                  43 34 38 32 35 45 45 00 00 )                      // C4825EE..
  .custom instance void [mscorlib]System.Runtime.InteropServices.ComConversionLossAttribute::.ctor() = ( 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 ) 
  .method public hidebysig newslot abstract virtual 
          instance int32  CompareElements([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) el1,
                                          [in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) el2) runtime managed internalcall
  {
  } // end of method IUIAutomation::CompareElements

  .method public hidebysig newslot abstract virtual 
          instance int32  CompareRuntimeIds([in] class [mscorlib]System.Array  marshal( safearray int) runtimeId1,
                                            [in] class [mscorlib]System.Array  marshal( safearray int) runtimeId2) runtime managed internalcall
  {
  } // end of method IUIAutomation::CompareRuntimeIds

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetRootElement() runtime managed internalcall
  {
  } // end of method IUIAutomation::GetRootElement

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromHandle([in] native int hwnd) runtime managed internalcall
  {
  } // end of method IUIAutomation::ElementFromHandle

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromPoint([in] valuetype UIAutomationClient.tagPOINT pt) runtime managed internalcall
  {
  } // end of method IUIAutomation::ElementFromPoint

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetFocusedElement() runtime managed internalcall
  {
  } // end of method IUIAutomation::GetFocusedElement

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetRootElementBuildCache([in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomation::GetRootElementBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromHandleBuildCache([in] native int hwnd,
                                      [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomation::ElementFromHandleBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromPointBuildCache([in] valuetype UIAutomationClient.tagPOINT pt,
                                     [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomation::ElementFromPointBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetFocusedElementBuildCache([in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomation::GetFocusedElementBuildCache

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationTreeWalker 
          marshal( interface ) 
          CreateTreeWalker([in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) pCondition) runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateTreeWalker

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationTreeWalker 
          marshal( interface ) 
          get_ControlViewWalker() runtime managed internalcall
  {
  } // end of method IUIAutomation::get_ControlViewWalker

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationTreeWalker 
          marshal( interface ) 
          get_ContentViewWalker() runtime managed internalcall
  {
  } // end of method IUIAutomation::get_ContentViewWalker

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationTreeWalker 
          marshal( interface ) 
          get_RawViewWalker() runtime managed internalcall
  {
  } // end of method IUIAutomation::get_RawViewWalker

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          get_RawViewCondition() runtime managed internalcall
  {
  } // end of method IUIAutomation::get_RawViewCondition

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          get_ControlViewCondition() runtime managed internalcall
  {
  } // end of method IUIAutomation::get_ControlViewCondition

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          get_ContentViewCondition() runtime managed internalcall
  {
  } // end of method IUIAutomation::get_ContentViewCondition

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCacheRequest 
          marshal( interface ) 
          CreateCacheRequest() runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateCacheRequest

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateTrueCondition() runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateTrueCondition

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateFalseCondition() runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateFalseCondition

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreatePropertyCondition([in] int32 propertyId,
                                  [in] object  marshal( struct) 'value') runtime managed internalcall
  {
  } // end of method IUIAutomation::CreatePropertyCondition

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreatePropertyConditionEx([in] int32 propertyId,
                                    [in] object  marshal( struct) 'value',
                                    [in] valuetype UIAutomationClient.PropertyConditionFlags 'flags') runtime managed internalcall
  {
  } // end of method IUIAutomation::CreatePropertyConditionEx

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateAndCondition([in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition1,
                             [in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition2) runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateAndCondition

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateAndConditionFromArray([in] class [mscorlib]System.Array  marshal( safearray iunknown,
                                       "UIAutomationClient.IUIAutomationCondition") conditions) runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateAndConditionFromArray

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateAndConditionFromNativeArray([in] class UIAutomationClient.IUIAutomationCondition&  marshal( interface ) conditions,
                                            [in] int32 conditionCount) runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateAndConditionFromNativeArray

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateOrCondition([in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition1,
                            [in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition2) runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateOrCondition

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateOrConditionFromArray([in] class [mscorlib]System.Array  marshal( safearray iunknown,
                                      "UIAutomationClient.IUIAutomationCondition") conditions) runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateOrConditionFromArray

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateOrConditionFromNativeArray([in] class UIAutomationClient.IUIAutomationCondition&  marshal( interface ) conditions,
                                           [in] int32 conditionCount) runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateOrConditionFromNativeArray

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateNotCondition([in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition) runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateNotCondition

  .method public hidebysig newslot abstract virtual 
          instance void  AddAutomationEventHandler([in] int32 eventId,
                                                   [in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                   [in] valuetype UIAutomationClient.TreeScope scope,
                                                   [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                   [in] class UIAutomationClient.IUIAutomationEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
  } // end of method IUIAutomation::AddAutomationEventHandler

  .method public hidebysig newslot abstract virtual 
          instance void  RemoveAutomationEventHandler([in] int32 eventId,
                                                      [in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                      [in] class UIAutomationClient.IUIAutomationEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
  } // end of method IUIAutomation::RemoveAutomationEventHandler

  .method public hidebysig newslot abstract virtual 
          instance void  AddPropertyChangedEventHandlerNativeArray([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                                   [in] valuetype UIAutomationClient.TreeScope scope,
                                                                   [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                                   [in] class UIAutomationClient.IUIAutomationPropertyChangedEventHandler  marshal( interface ) 'handler',
                                                                   [in] int32& propertyArray,
                                                                   [in] int32 propertyCount) runtime managed internalcall
  {
  } // end of method IUIAutomation::AddPropertyChangedEventHandlerNativeArray

  .method public hidebysig newslot abstract virtual 
          instance void  AddPropertyChangedEventHandler([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                        [in] valuetype UIAutomationClient.TreeScope scope,
                                                        [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                        [in] class UIAutomationClient.IUIAutomationPropertyChangedEventHandler  marshal( interface ) 'handler',
                                                        [in] class [mscorlib]System.Array  marshal( safearray int) propertyArray) runtime managed internalcall
  {
  } // end of method IUIAutomation::AddPropertyChangedEventHandler

  .method public hidebysig newslot abstract virtual 
          instance void  RemovePropertyChangedEventHandler([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                           [in] class UIAutomationClient.IUIAutomationPropertyChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
  } // end of method IUIAutomation::RemovePropertyChangedEventHandler

  .method public hidebysig newslot abstract virtual 
          instance void  AddStructureChangedEventHandler([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                         [in] valuetype UIAutomationClient.TreeScope scope,
                                                         [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                         [in] class UIAutomationClient.IUIAutomationStructureChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
  } // end of method IUIAutomation::AddStructureChangedEventHandler

  .method public hidebysig newslot abstract virtual 
          instance void  RemoveStructureChangedEventHandler([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                            [in] class UIAutomationClient.IUIAutomationStructureChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
  } // end of method IUIAutomation::RemoveStructureChangedEventHandler

  .method public hidebysig newslot abstract virtual 
          instance void  AddFocusChangedEventHandler([in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                     [in] class UIAutomationClient.IUIAutomationFocusChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
  } // end of method IUIAutomation::AddFocusChangedEventHandler

  .method public hidebysig newslot abstract virtual 
          instance void  RemoveFocusChangedEventHandler([in] class UIAutomationClient.IUIAutomationFocusChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
  } // end of method IUIAutomation::RemoveFocusChangedEventHandler

  .method public hidebysig newslot abstract virtual 
          instance void  RemoveAllEventHandlers() runtime managed internalcall
  {
  } // end of method IUIAutomation::RemoveAllEventHandlers

  .method public hidebysig newslot abstract virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray int) 
          IntNativeArrayToSafeArray([in] int32& 'array',
                                    [in] int32 arrayCount) runtime managed internalcall
  {
  } // end of method IUIAutomation::IntNativeArrayToSafeArray

  .method public hidebysig newslot abstract virtual 
          instance int32  IntSafeArrayToNativeArray([in] class [mscorlib]System.Array  marshal( safearray int) intArray,
                                                    [out] native int 'array') runtime managed internalcall
  {
  } // end of method IUIAutomation::IntSafeArrayToNativeArray

  .method public hidebysig newslot abstract virtual 
          instance object 
          marshal( struct) 
          RectToVariant([in] valuetype UIAutomationClient.tagRECT rc) runtime managed internalcall
  {
  } // end of method IUIAutomation::RectToVariant

  .method public hidebysig newslot abstract virtual 
          instance valuetype UIAutomationClient.tagRECT 
          VariantToRect([in] object  marshal( struct) var) runtime managed internalcall
  {
  } // end of method IUIAutomation::VariantToRect

  .method public hidebysig newslot abstract virtual 
          instance int32  SafeArrayToRectNativeArray([in] class [mscorlib]System.Array  marshal( safearray float64) rects,
                                                     [out] native int rectArray) runtime managed internalcall
  {
  } // end of method IUIAutomation::SafeArrayToRectNativeArray

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationProxyFactoryEntry 
          marshal( interface ) 
          CreateProxyFactoryEntry([in] class UIAutomationClient.IUIAutomationProxyFactory  marshal( interface ) factory) runtime managed internalcall
  {
  } // end of method IUIAutomation::CreateProxyFactoryEntry

  .method public hidebysig newslot specialname abstract virtual 
          instance class UIAutomationClient.IUIAutomationProxyFactoryMapping 
          marshal( interface ) 
          get_ProxyFactoryMapping() runtime managed internalcall
  {
  } // end of method IUIAutomation::get_ProxyFactoryMapping

  .method public hidebysig newslot abstract virtual 
          instance string 
          marshal( bstr) 
          GetPropertyProgrammaticName([in] int32 'property') runtime managed internalcall
  {
  } // end of method IUIAutomation::GetPropertyProgrammaticName

  .method public hidebysig newslot abstract virtual 
          instance string 
          marshal( bstr) 
          GetPatternProgrammaticName([in] int32 pattern) runtime managed internalcall
  {
  } // end of method IUIAutomation::GetPatternProgrammaticName

  .method public hidebysig newslot abstract virtual 
          instance void  PollForPotentialSupportedPatterns([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) pElement,
                                                           [out] class [mscorlib]System.Array&  marshal( safearray int) patternIds,
                                                           [out] class [mscorlib]System.Array&  marshal( safearray bstr) patternNames) runtime managed internalcall
  {
  } // end of method IUIAutomation::PollForPotentialSupportedPatterns

  .method public hidebysig newslot abstract virtual 
          instance void  PollForPotentialSupportedProperties([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) pElement,
                                                             [out] class [mscorlib]System.Array&  marshal( safearray int) propertyIds,
                                                             [out] class [mscorlib]System.Array&  marshal( safearray bstr) propertyNames) runtime managed internalcall
  {
  } // end of method IUIAutomation::PollForPotentialSupportedProperties

  .method public hidebysig newslot abstract virtual 
          instance int32  CheckNotSupported([in] object  marshal( struct) 'value') runtime managed internalcall
  {
  } // end of method IUIAutomation::CheckNotSupported

  .method public hidebysig newslot specialname abstract virtual 
          instance object 
          marshal( iunknown ) 
          get_ReservedNotSupportedValue() runtime managed internalcall
  {
  } // end of method IUIAutomation::get_ReservedNotSupportedValue

  .method public hidebysig newslot specialname abstract virtual 
          instance object 
          marshal( iunknown ) 
          get_ReservedMixedAttributeValue() runtime managed internalcall
  {
  } // end of method IUIAutomation::get_ReservedMixedAttributeValue

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromIAccessible([in] class UIAutomationClient.IAccessible  marshal( interface ) accessible,
                                 [in] int32 childId) runtime managed internalcall
  {
  } // end of method IUIAutomation::ElementFromIAccessible

  .method public hidebysig newslot abstract virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromIAccessibleBuildCache([in] class UIAutomationClient.IAccessible  marshal( interface ) accessible,
                                           [in] int32 childId,
                                           [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
  } // end of method IUIAutomation::ElementFromIAccessibleBuildCache

  .property class UIAutomationClient.IUIAutomationTreeWalker
          ControlViewWalker()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationTreeWalker UIAutomationClient.IUIAutomation::get_ControlViewWalker()
  } // end of property IUIAutomation::ControlViewWalker
  .property class UIAutomationClient.IUIAutomationTreeWalker
          ContentViewWalker()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationTreeWalker UIAutomationClient.IUIAutomation::get_ContentViewWalker()
  } // end of property IUIAutomation::ContentViewWalker
  .property class UIAutomationClient.IUIAutomationTreeWalker
          RawViewWalker()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationTreeWalker UIAutomationClient.IUIAutomation::get_RawViewWalker()
  } // end of property IUIAutomation::RawViewWalker
  .property class UIAutomationClient.IUIAutomationCondition
          RawViewCondition()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationCondition UIAutomationClient.IUIAutomation::get_RawViewCondition()
  } // end of property IUIAutomation::RawViewCondition
  .property class UIAutomationClient.IUIAutomationCondition
          ControlViewCondition()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationCondition UIAutomationClient.IUIAutomation::get_ControlViewCondition()
  } // end of property IUIAutomation::ControlViewCondition
  .property class UIAutomationClient.IUIAutomationCondition
          ContentViewCondition()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 10 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationCondition UIAutomationClient.IUIAutomation::get_ContentViewCondition()
  } // end of property IUIAutomation::ContentViewCondition
  .property class UIAutomationClient.IUIAutomationProxyFactoryMapping
          ProxyFactoryMapping()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 2D 00 01 60 00 00 )                         // ..-..`..
    .get instance class UIAutomationClient.IUIAutomationProxyFactoryMapping UIAutomationClient.IUIAutomation::get_ProxyFactoryMapping()
  } // end of property IUIAutomation::ProxyFactoryMapping
  .property object ReservedNotSupportedValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 33 00 01 60 00 00 )                         // ..3..`..
    .get instance object UIAutomationClient.IUIAutomation::get_ReservedNotSupportedValue()
  } // end of property IUIAutomation::ReservedNotSupportedValue
  .property object ReservedMixedAttributeValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 34 00 01 60 00 00 )                         // ..4..`..
    .get instance object UIAutomationClient.IUIAutomation::get_ReservedMixedAttributeValue()
  } // end of property IUIAutomation::ReservedMixedAttributeValue
} // end of class UIAutomationClient.IUIAutomation

.class public auto ansi import UIAutomationClient.CUIAutomationClass
       extends [mscorlib]System.Object
       implements UIAutomationClient.IUIAutomation,
                  UIAutomationClient.CUIAutomation
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.ClassInterfaceAttribute::.ctor(int16) = ( 01 00 00 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 46 46 34 38 44 42 41 34 2D 36 30 45 46   // ..$FF48DBA4-60EF
                                                                                                  2D 34 32 30 31 2D 41 41 38 37 2D 35 34 31 30 33   // -4201-AA87-54103
                                                                                                  45 45 46 35 39 34 45 00 00 )                      // EEF594E..
  .custom instance void [mscorlib]System.Runtime.InteropServices.ComConversionLossAttribute::.ctor() = ( 01 00 00 00 ) 
  .custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 00 00 00 ) 
  .method public specialname rtspecialname 
          instance void  .ctor() runtime managed internalcall
  {
  } // end of method CUIAutomationClass::.ctor

  .method public hidebysig newslot virtual 
          instance int32  CompareElements([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) el1,
                                          [in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) el2) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CompareElements
  } // end of method CUIAutomationClass::CompareElements

  .method public hidebysig newslot virtual 
          instance int32  CompareRuntimeIds([in] class [mscorlib]System.Array  marshal( safearray int) runtimeId1,
                                            [in] class [mscorlib]System.Array  marshal( safearray int) runtimeId2) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CompareRuntimeIds
  } // end of method CUIAutomationClass::CompareRuntimeIds

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetRootElement() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::GetRootElement
  } // end of method CUIAutomationClass::GetRootElement

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromHandle([in] native int hwnd) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::ElementFromHandle
  } // end of method CUIAutomationClass::ElementFromHandle

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromPoint([in] valuetype UIAutomationClient.tagPOINT pt) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::ElementFromPoint
  } // end of method CUIAutomationClass::ElementFromPoint

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetFocusedElement() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::GetFocusedElement
  } // end of method CUIAutomationClass::GetFocusedElement

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetRootElementBuildCache([in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::GetRootElementBuildCache
  } // end of method CUIAutomationClass::GetRootElementBuildCache

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromHandleBuildCache([in] native int hwnd,
                                      [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::ElementFromHandleBuildCache
  } // end of method CUIAutomationClass::ElementFromHandleBuildCache

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromPointBuildCache([in] valuetype UIAutomationClient.tagPOINT pt,
                                     [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::ElementFromPointBuildCache
  } // end of method CUIAutomationClass::ElementFromPointBuildCache

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          GetFocusedElementBuildCache([in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::GetFocusedElementBuildCache
  } // end of method CUIAutomationClass::GetFocusedElementBuildCache

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationTreeWalker 
          marshal( interface ) 
          CreateTreeWalker([in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) pCondition) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateTreeWalker
  } // end of method CUIAutomationClass::CreateTreeWalker

  .method public hidebysig newslot specialname virtual 
          instance class UIAutomationClient.IUIAutomationTreeWalker 
          marshal( interface ) 
          get_ControlViewWalker() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::get_ControlViewWalker
  } // end of method CUIAutomationClass::get_ControlViewWalker

  .method public hidebysig newslot specialname virtual 
          instance class UIAutomationClient.IUIAutomationTreeWalker 
          marshal( interface ) 
          get_ContentViewWalker() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::get_ContentViewWalker
  } // end of method CUIAutomationClass::get_ContentViewWalker

  .method public hidebysig newslot specialname virtual 
          instance class UIAutomationClient.IUIAutomationTreeWalker 
          marshal( interface ) 
          get_RawViewWalker() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::get_RawViewWalker
  } // end of method CUIAutomationClass::get_RawViewWalker

  .method public hidebysig newslot specialname virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          get_RawViewCondition() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::get_RawViewCondition
  } // end of method CUIAutomationClass::get_RawViewCondition

  .method public hidebysig newslot specialname virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          get_ControlViewCondition() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::get_ControlViewCondition
  } // end of method CUIAutomationClass::get_ControlViewCondition

  .method public hidebysig newslot specialname virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          get_ContentViewCondition() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::get_ContentViewCondition
  } // end of method CUIAutomationClass::get_ContentViewCondition

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCacheRequest 
          marshal( interface ) 
          CreateCacheRequest() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateCacheRequest
  } // end of method CUIAutomationClass::CreateCacheRequest

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateTrueCondition() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateTrueCondition
  } // end of method CUIAutomationClass::CreateTrueCondition

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateFalseCondition() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateFalseCondition
  } // end of method CUIAutomationClass::CreateFalseCondition

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreatePropertyCondition([in] int32 propertyId,
                                  [in] object  marshal( struct) 'value') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreatePropertyCondition
  } // end of method CUIAutomationClass::CreatePropertyCondition

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreatePropertyConditionEx([in] int32 propertyId,
                                    [in] object  marshal( struct) 'value',
                                    [in] valuetype UIAutomationClient.PropertyConditionFlags 'flags') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreatePropertyConditionEx
  } // end of method CUIAutomationClass::CreatePropertyConditionEx

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateAndCondition([in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition1,
                             [in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition2) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateAndCondition
  } // end of method CUIAutomationClass::CreateAndCondition

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateAndConditionFromArray([in] class [mscorlib]System.Array  marshal( safearray iunknown,
                                       "UIAutomationClient.IUIAutomationCondition") conditions) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateAndConditionFromArray
  } // end of method CUIAutomationClass::CreateAndConditionFromArray

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateAndConditionFromNativeArray([in] class UIAutomationClient.IUIAutomationCondition&  marshal( interface ) conditions,
                                            [in] int32 conditionCount) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateAndConditionFromNativeArray
  } // end of method CUIAutomationClass::CreateAndConditionFromNativeArray

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateOrCondition([in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition1,
                            [in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition2) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateOrCondition
  } // end of method CUIAutomationClass::CreateOrCondition

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateOrConditionFromArray([in] class [mscorlib]System.Array  marshal( safearray iunknown,
                                      "UIAutomationClient.IUIAutomationCondition") conditions) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateOrConditionFromArray
  } // end of method CUIAutomationClass::CreateOrConditionFromArray

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateOrConditionFromNativeArray([in] class UIAutomationClient.IUIAutomationCondition&  marshal( interface ) conditions,
                                           [in] int32 conditionCount) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateOrConditionFromNativeArray
  } // end of method CUIAutomationClass::CreateOrConditionFromNativeArray

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationCondition 
          marshal( interface ) 
          CreateNotCondition([in] class UIAutomationClient.IUIAutomationCondition  marshal( interface ) condition) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateNotCondition
  } // end of method CUIAutomationClass::CreateNotCondition

  .method public hidebysig newslot virtual 
          instance void  AddAutomationEventHandler([in] int32 eventId,
                                                   [in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                   [in] valuetype UIAutomationClient.TreeScope scope,
                                                   [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                   [in] class UIAutomationClient.IUIAutomationEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::AddAutomationEventHandler
  } // end of method CUIAutomationClass::AddAutomationEventHandler

  .method public hidebysig newslot virtual 
          instance void  RemoveAutomationEventHandler([in] int32 eventId,
                                                      [in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                      [in] class UIAutomationClient.IUIAutomationEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::RemoveAutomationEventHandler
  } // end of method CUIAutomationClass::RemoveAutomationEventHandler

  .method public hidebysig newslot virtual 
          instance void  AddPropertyChangedEventHandlerNativeArray([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                                   [in] valuetype UIAutomationClient.TreeScope scope,
                                                                   [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                                   [in] class UIAutomationClient.IUIAutomationPropertyChangedEventHandler  marshal( interface ) 'handler',
                                                                   [in] int32& propertyArray,
                                                                   [in] int32 propertyCount) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::AddPropertyChangedEventHandlerNativeArray
  } // end of method CUIAutomationClass::AddPropertyChangedEventHandlerNativeArray

  .method public hidebysig newslot virtual 
          instance void  AddPropertyChangedEventHandler([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                        [in] valuetype UIAutomationClient.TreeScope scope,
                                                        [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                        [in] class UIAutomationClient.IUIAutomationPropertyChangedEventHandler  marshal( interface ) 'handler',
                                                        [in] class [mscorlib]System.Array  marshal( safearray int) propertyArray) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::AddPropertyChangedEventHandler
  } // end of method CUIAutomationClass::AddPropertyChangedEventHandler

  .method public hidebysig newslot virtual 
          instance void  RemovePropertyChangedEventHandler([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                           [in] class UIAutomationClient.IUIAutomationPropertyChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::RemovePropertyChangedEventHandler
  } // end of method CUIAutomationClass::RemovePropertyChangedEventHandler

  .method public hidebysig newslot virtual 
          instance void  AddStructureChangedEventHandler([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                         [in] valuetype UIAutomationClient.TreeScope scope,
                                                         [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                         [in] class UIAutomationClient.IUIAutomationStructureChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::AddStructureChangedEventHandler
  } // end of method CUIAutomationClass::AddStructureChangedEventHandler

  .method public hidebysig newslot virtual 
          instance void  RemoveStructureChangedEventHandler([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) element,
                                                            [in] class UIAutomationClient.IUIAutomationStructureChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::RemoveStructureChangedEventHandler
  } // end of method CUIAutomationClass::RemoveStructureChangedEventHandler

  .method public hidebysig newslot virtual 
          instance void  AddFocusChangedEventHandler([in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest,
                                                     [in] class UIAutomationClient.IUIAutomationFocusChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::AddFocusChangedEventHandler
  } // end of method CUIAutomationClass::AddFocusChangedEventHandler

  .method public hidebysig newslot virtual 
          instance void  RemoveFocusChangedEventHandler([in] class UIAutomationClient.IUIAutomationFocusChangedEventHandler  marshal( interface ) 'handler') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::RemoveFocusChangedEventHandler
  } // end of method CUIAutomationClass::RemoveFocusChangedEventHandler

  .method public hidebysig newslot virtual 
          instance void  RemoveAllEventHandlers() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::RemoveAllEventHandlers
  } // end of method CUIAutomationClass::RemoveAllEventHandlers

  .method public hidebysig newslot virtual 
          instance class [mscorlib]System.Array 
          marshal( safearray int) 
          IntNativeArrayToSafeArray([in] int32& 'array',
                                    [in] int32 arrayCount) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::IntNativeArrayToSafeArray
  } // end of method CUIAutomationClass::IntNativeArrayToSafeArray

  .method public hidebysig newslot virtual 
          instance int32  IntSafeArrayToNativeArray([in] class [mscorlib]System.Array  marshal( safearray int) intArray,
                                                    [out] native int 'array') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::IntSafeArrayToNativeArray
  } // end of method CUIAutomationClass::IntSafeArrayToNativeArray

  .method public hidebysig newslot virtual 
          instance object 
          marshal( struct) 
          RectToVariant([in] valuetype UIAutomationClient.tagRECT rc) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::RectToVariant
  } // end of method CUIAutomationClass::RectToVariant

  .method public hidebysig newslot virtual 
          instance valuetype UIAutomationClient.tagRECT 
          VariantToRect([in] object  marshal( struct) var) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::VariantToRect
  } // end of method CUIAutomationClass::VariantToRect

  .method public hidebysig newslot virtual 
          instance int32  SafeArrayToRectNativeArray([in] class [mscorlib]System.Array  marshal( safearray float64) rects,
                                                     [out] native int rectArray) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::SafeArrayToRectNativeArray
  } // end of method CUIAutomationClass::SafeArrayToRectNativeArray

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationProxyFactoryEntry 
          marshal( interface ) 
          CreateProxyFactoryEntry([in] class UIAutomationClient.IUIAutomationProxyFactory  marshal( interface ) factory) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CreateProxyFactoryEntry
  } // end of method CUIAutomationClass::CreateProxyFactoryEntry

  .method public hidebysig newslot specialname virtual 
          instance class UIAutomationClient.IUIAutomationProxyFactoryMapping 
          marshal( interface ) 
          get_ProxyFactoryMapping() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::get_ProxyFactoryMapping
  } // end of method CUIAutomationClass::get_ProxyFactoryMapping

  .method public hidebysig newslot virtual 
          instance string 
          marshal( bstr) 
          GetPropertyProgrammaticName([in] int32 'property') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::GetPropertyProgrammaticName
  } // end of method CUIAutomationClass::GetPropertyProgrammaticName

  .method public hidebysig newslot virtual 
          instance string 
          marshal( bstr) 
          GetPatternProgrammaticName([in] int32 pattern) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::GetPatternProgrammaticName
  } // end of method CUIAutomationClass::GetPatternProgrammaticName

  .method public hidebysig newslot virtual 
          instance void  PollForPotentialSupportedPatterns([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) pElement,
                                                           [out] class [mscorlib]System.Array&  marshal( safearray int) patternIds,
                                                           [out] class [mscorlib]System.Array&  marshal( safearray bstr) patternNames) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::PollForPotentialSupportedPatterns
  } // end of method CUIAutomationClass::PollForPotentialSupportedPatterns

  .method public hidebysig newslot virtual 
          instance void  PollForPotentialSupportedProperties([in] class UIAutomationClient.IUIAutomationElement  marshal( interface ) pElement,
                                                             [out] class [mscorlib]System.Array&  marshal( safearray int) propertyIds,
                                                             [out] class [mscorlib]System.Array&  marshal( safearray bstr) propertyNames) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::PollForPotentialSupportedProperties
  } // end of method CUIAutomationClass::PollForPotentialSupportedProperties

  .method public hidebysig newslot virtual 
          instance int32  CheckNotSupported([in] object  marshal( struct) 'value') runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::CheckNotSupported
  } // end of method CUIAutomationClass::CheckNotSupported

  .method public hidebysig newslot specialname virtual 
          instance object 
          marshal( iunknown ) 
          get_ReservedNotSupportedValue() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::get_ReservedNotSupportedValue
  } // end of method CUIAutomationClass::get_ReservedNotSupportedValue

  .method public hidebysig newslot specialname virtual 
          instance object 
          marshal( iunknown ) 
          get_ReservedMixedAttributeValue() runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::get_ReservedMixedAttributeValue
  } // end of method CUIAutomationClass::get_ReservedMixedAttributeValue

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromIAccessible([in] class UIAutomationClient.IAccessible  marshal( interface ) accessible,
                                 [in] int32 childId) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::ElementFromIAccessible
  } // end of method CUIAutomationClass::ElementFromIAccessible

  .method public hidebysig newslot virtual 
          instance class UIAutomationClient.IUIAutomationElement 
          marshal( interface ) 
          ElementFromIAccessibleBuildCache([in] class UIAutomationClient.IAccessible  marshal( interface ) accessible,
                                           [in] int32 childId,
                                           [in] class UIAutomationClient.IUIAutomationCacheRequest  marshal( interface ) cacheRequest) runtime managed internalcall
  {
    .override UIAutomationClient.IUIAutomation::ElementFromIAccessibleBuildCache
  } // end of method CUIAutomationClass::ElementFromIAccessibleBuildCache

  .property class UIAutomationClient.IUIAutomationTreeWalker
          ControlViewWalker()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationTreeWalker UIAutomationClient.CUIAutomationClass::get_ControlViewWalker()
  } // end of property CUIAutomationClass::ControlViewWalker
  .property class UIAutomationClient.IUIAutomationTreeWalker
          ContentViewWalker()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationTreeWalker UIAutomationClient.CUIAutomationClass::get_ContentViewWalker()
  } // end of property CUIAutomationClass::ContentViewWalker
  .property class UIAutomationClient.IUIAutomationTreeWalker
          RawViewWalker()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationTreeWalker UIAutomationClient.CUIAutomationClass::get_RawViewWalker()
  } // end of property CUIAutomationClass::RawViewWalker
  .property class UIAutomationClient.IUIAutomationCondition
          RawViewCondition()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationCondition UIAutomationClient.CUIAutomationClass::get_RawViewCondition()
  } // end of property CUIAutomationClass::RawViewCondition
  .property class UIAutomationClient.IUIAutomationCondition
          ControlViewCondition()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationCondition UIAutomationClient.CUIAutomationClass::get_ControlViewCondition()
  } // end of property CUIAutomationClass::ControlViewCondition
  .property class UIAutomationClient.IUIAutomationCondition
          ContentViewCondition()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 10 00 01 60 00 00 )                         // .....`..
    .get instance class UIAutomationClient.IUIAutomationCondition UIAutomationClient.CUIAutomationClass::get_ContentViewCondition()
  } // end of property CUIAutomationClass::ContentViewCondition
  .property class UIAutomationClient.IUIAutomationProxyFactoryMapping
          ProxyFactoryMapping()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 2D 00 01 60 00 00 )                         // ..-..`..
    .get instance class UIAutomationClient.IUIAutomationProxyFactoryMapping UIAutomationClient.CUIAutomationClass::get_ProxyFactoryMapping()
  } // end of property CUIAutomationClass::ProxyFactoryMapping
  .property object ReservedNotSupportedValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 33 00 01 60 00 00 )                         // ..3..`..
    .get instance object UIAutomationClient.CUIAutomationClass::get_ReservedNotSupportedValue()
  } // end of property CUIAutomationClass::ReservedNotSupportedValue
  .property object ReservedMixedAttributeValue()
  {
    .custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 34 00 01 60 00 00 )                         // ..4..`..
    .get instance object UIAutomationClient.CUIAutomationClass::get_ReservedMixedAttributeValue()
  } // end of property CUIAutomationClass::ReservedMixedAttributeValue
} // end of class UIAutomationClient.CUIAutomationClass

.class interface public abstract auto ansi import UIAutomationClient.CUIAutomation
       implements UIAutomationClient.IUIAutomation
{
  .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( 01 00 24 33 30 43 42 45 35 37 44 2D 44 39 44 30   // ..$30CBE57D-D9D0
                                                                                                  2D 34 35 32 41 2D 41 42 31 33 2D 37 41 43 35 41   // -452A-AB13-7AC5A
                                                                                                  43 34 38 32 35 45 45 00 00 )                      // C4825EE..
  .custom instance void [mscorlib]System.Runtime.InteropServices.CoClassAttribute::.ctor(class [mscorlib]System.Type) = ( 01 00 25 55 49 41 75 74 6F 6D 61 74 69 6F 6E 43   // ..%UIAutomationC
                                                                                                                          6C 69 65 6E 74 2E 43 55 49 41 75 74 6F 6D 61 74   // lient.CUIAutomat
                                                                                                                          69 6F 6E 43 6C 61 73 73 00 00 )                   // ionClass..
} // end of class UIAutomationClient.CUIAutomation


// =============================================================

// *********** DISASSEMBLY COMPLETE ***********************
