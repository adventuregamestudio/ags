using System.Windows.Forms;
using NUnit.Framework;

namespace AGS.Editor
{
    [TestFixture]
    public class FreePanControlTests
    {
        private FreePanControlTestableWrapper _pane;

        [SetUp]
        public void SetUp()
        {
            _pane = new FreePanControlTestableWrapper();
        }

        [Test]
        public void IsPanningDefaultsToFalse()
        {
            Assert.That(_pane.IsPanning, Is.False);
        }

        [Test]
        public void IsPanningIsTrueOnMiddleMouseClick()
        {
            _pane.OnMouseDown(new MouseEventArgs(MouseButtons.Middle, 1, 5, 5, 0));
            Assert.That(_pane.IsPanning, Is.True);
        }

        [Test]
        public void PanGrabbedInvokesOnMiddleMouseClick()
        {
            bool panGrabbedInvoked = false;
            _pane.PanGrabbed += (s, e) => panGrabbedInvoked = true;
            _pane.OnMouseDown(new MouseEventArgs(MouseButtons.Middle, 1, 5, 5, 0));
            Assert.That(panGrabbedInvoked, Is.True);
        }

        [Test]
        public void IsPanningReturnsToFalseOnPan()
        {
            _pane.OnMouseDown(new MouseEventArgs(MouseButtons.Middle, 1, 5, 5, 0));
            Assert.That(_pane.IsPanning, Is.True);
            _pane.OnMouseMove(new MouseEventArgs(MouseButtons.Middle, 1, 4, 4, 0));
            Assert.That(_pane.IsPanning, Is.True);
            _pane.OnMouseUp(new MouseEventArgs(MouseButtons.Middle, 1, 3, 3, 0));
            Assert.That(_pane.IsPanning, Is.False);
        }

        [Test]
        public void CursorIsModifiedAndReturned()
        {
            _pane.Cursor = Cursors.IBeam;
            Assert.That(_pane.Cursor, Is.EqualTo(Cursors.IBeam));
            _pane.OnMouseDown(new MouseEventArgs(MouseButtons.Middle, 1, 5, 5, 0));
            Assert.That(_pane.Cursor, Is.EqualTo(_pane.PanCursor));
            _pane.OnMouseUp(new MouseEventArgs(MouseButtons.Middle, 1, 3, 3, 0));
            Assert.That(_pane.Cursor, Is.EqualTo(Cursors.Default));
            // this pane always go back to default cursor, if you want to change it to other cursor
            // use OnPanRelease to change the cursor to the state desired.
            // this prevents storing a "state" WaitCursor and restoring to it when it's uneeded.
        }

        private class FreePanControlTestableWrapper : FreePanControl
        {
            public new void OnMouseDown(MouseEventArgs e) => base.OnMouseDown(e);
            public new void OnMouseUp(MouseEventArgs e) => base.OnMouseUp(e);
            public new void OnMouseMove(MouseEventArgs e) => base.OnMouseMove(e);
        }
    }
}
