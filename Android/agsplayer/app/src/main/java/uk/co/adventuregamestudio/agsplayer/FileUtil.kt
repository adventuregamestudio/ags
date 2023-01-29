package uk.co.adventuregamestudio.agsplayer

import android.annotation.TargetApi
import android.content.Context
import android.net.Uri
import android.os.Build
import android.os.storage.StorageManager
import android.os.storage.StorageVolume
import android.provider.DocumentsContract
import androidx.annotation.Nullable
import java.io.File
import java.lang.Exception
import java.lang.reflect.Array
import java.lang.reflect.Method

// lifted from https://stackoverflow.com/questions/34927748/android-5-0-documentfile-from-tree-uri/36162691#36162691

object FileUtil {
    private const val PRIMARY_VOLUME_NAME = "primary"
    @Nullable
    fun getFullPathFromTreeUri(@Nullable treeUri: Uri?, con: Context): String? {
        if (treeUri == null) return null
        var volumePath = getVolumePath(getVolumeIdFromTreeUri(treeUri), con)
        if (volumePath == null) return File.separator
        if (volumePath.endsWith(File.separator)) volumePath =
            volumePath.substring(0, volumePath.length - 1)
        var documentPath = getDocumentPathFromTreeUri(treeUri)
        if (documentPath != null) {
            if (documentPath.endsWith(File.separator)) documentPath =
                documentPath!!.substring(0, documentPath.length - 1)
        }
        return if (documentPath!!.length > 0) {
            if (documentPath.startsWith(File.separator)) volumePath + documentPath else volumePath + File.separator.toString() + documentPath
        } else volumePath
    }

    private fun getVolumePath(volumeId: String?, context: Context): String? {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) return null
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) getVolumePathForAndroid11AndAbove(
            volumeId,
            context
        )
        else return getVolumePathBeforeAndroid11(volumeId, context)
    }

    private fun getVolumePathBeforeAndroid11(volumeId: String?, context: Context): String? {
        return try {
            val mStorageManager: StorageManager =
                context.getSystemService(Context.STORAGE_SERVICE) as StorageManager
            val storageVolumeClazz = Class.forName("android.os.storage.StorageVolume")
            val getVolumeList: Method = mStorageManager.javaClass.getMethod("getVolumeList")
            val getUuid: Method = storageVolumeClazz.getMethod("getUuid")
            val getPath: Method = storageVolumeClazz.getMethod("getPath")
            val isPrimary: Method = storageVolumeClazz.getMethod("isPrimary")
            val result: Any = getVolumeList.invoke(mStorageManager)
            val length: Int = Array.getLength(result)
            for (i in 0 until length) {
                val storageVolumeElement: Any = Array.get(result, i)
                val primary = isPrimary.invoke(storageVolumeElement) as Boolean
                if (primary && PRIMARY_VOLUME_NAME == volumeId) // primary volume?
                    return getPath.invoke(storageVolumeElement) as String?
                val uuid = getUuid.invoke(storageVolumeElement) as String
                if (uuid != null && uuid == volumeId) // other volumes?
                    return getPath.invoke(storageVolumeElement) as String?
            }
            // not found.
            null
        } catch (ex: Exception) {
            null
        }
    }

    @TargetApi(Build.VERSION_CODES.R)
    private fun getVolumePathForAndroid11AndAbove(volumeId: String?, context: Context): String? {
        return try {
            val mStorageManager: StorageManager =
                context.getSystemService(Context.STORAGE_SERVICE) as StorageManager
            val storageVolumes: List<StorageVolume> = mStorageManager.getStorageVolumes()
            for (storageVolume in storageVolumes) {
                // primary volume?
                if (storageVolume.isPrimary() && PRIMARY_VOLUME_NAME == volumeId) return storageVolume.getDirectory()
                    ?.getPath()

                // other volumes?
                val uuid: String = storageVolume.getUuid().toString()
                if (uuid != null && uuid == volumeId) return storageVolume.getDirectory()?.getPath()
            }
            // not found.
            null
        } catch (ex: Exception) {
            null
        }
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private fun getVolumeIdFromTreeUri(treeUri: Uri): String? {
        val docId: String = DocumentsContract.getTreeDocumentId(treeUri)
        val split = docId.split(":").toTypedArray()
        return if (split.size > 0) split[0] else null
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private fun getDocumentPathFromTreeUri(treeUri: Uri): String? {
        val docId: String = DocumentsContract.getTreeDocumentId(treeUri)
        val split = docId.split(":").toTypedArray()
        return if (split.size >= 2) split[1] else File.separator
    }
}