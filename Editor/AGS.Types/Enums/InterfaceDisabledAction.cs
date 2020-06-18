using System.ComponentModel;

namespace AGS.Types
{
    public enum InterfaceDisabledAction
    {
		[Description("Grey out all their controls")]
        GreyOut = 0,
		[Description("Hide all their controls")]
        GoBlack = 1,
		[Description("Display normally")]
        Unchanged = 2,
		[Description("Be hidden")]
        TurnOff = 3
    }
}
