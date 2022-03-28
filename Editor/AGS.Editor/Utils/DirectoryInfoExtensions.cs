using System;
using System.IO;

namespace AGS.Editor
{
    public static class DirectoryInfoExtensions
    {
        /// <summary>
        /// Copies everything from the source directory to the target directory.
        /// </summary>
        /// <param name="source">The source directory.</param>
        /// <param name="target">The target directory.</param>
        public static void CopyAll(this DirectoryInfo source, DirectoryInfo target)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));
            if (target == null) throw new ArgumentNullException(nameof(target));

            Directory.CreateDirectory(target.FullName);

            foreach (FileInfo fi in source.GetFiles())
                fi.CopyTo(Path.Combine(target.FullName, fi.Name), false);

            foreach (DirectoryInfo di in source.GetDirectories())
            {
                DirectoryInfo nextDi = target.CreateSubdirectory(di.Name);
                di.CopyAll(nextDi);
            }
        }

        /// <summary>
        /// Deletes a directory and its contents
        /// </summary>
        /// <param name="source">The source directory.</param>
        /// <param name="recursive">If true delete every file and sub directory.</param>
        public static void DeleteWithoutException(this DirectoryInfo source, bool recursive = true)
        {
            if (source == null) throw new ArgumentNullException(nameof(source));

            foreach (FileInfo fi in source.GetFiles())
            {
                try
                {
                    fi.Delete();
                }
                catch (Exception)
                {
                }
            }

            if (recursive)
            {
                foreach (DirectoryInfo di in source.GetDirectories())
                    di.DeleteWithoutException(recursive);
            }

            try
            {
                source.Delete(recursive);
            }
            catch (Exception)
            {
            }
        }
    }
}
