// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_IMAGE_WRITER_PRIVATE_OPERATION_H_
#define CHROME_BROWSER_EXTENSIONS_API_IMAGE_WRITER_PRIVATE_OPERATION_H_

#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/md5.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/timer/timer.h"
#include "chrome/browser/extensions/api/image_writer_private/image_writer_utility_client.h"
#include "chrome/common/extensions/api/image_writer_private.h"
#include "third_party/zlib/google/zip_reader.h"

namespace image_writer_api = extensions::api::image_writer_private;

namespace base {
class FilePath;
}  // namespace base

namespace extensions {
namespace image_writer {

const int kProgressComplete = 100;

class OperationManager;

// Encapsulates an operation being run on behalf of the
// OperationManager.  Construction of the operation does not start
// anything.  The operation's Start method should be called to start it, and
// then the Cancel method will stop it.  The operation will call back to the
// OperationManager periodically or on any significant event.
//
// Each stage of the operation is generally divided into three phases: Start,
// Run, Complete.  Start and Complete run on the UI thread and are responsible
// for advancing to the next stage and other UI interaction.  The Run phase does
// the work on the FILE thread and calls SendProgress or Error as appropriate.
//
// TODO(haven): This class is current refcounted because it is owned by the
// OperationManager on the UI thread but needs to do work on the FILE thread.
// There is probably a better way to organize this so that it can be represented
// by a WeakPtr, but those are not thread-safe.  Additionally, if destruction is
// done on the UI thread then that causes problems if any of the fields were
// allocated/accessed on the FILE thread.  http://crbug.com/344713
class Operation : public base::RefCountedThreadSafe<Operation> {
 public:
  typedef base::Callback<void(bool, const std::string&)> StartWriteCallback;
  typedef base::Callback<void(bool, const std::string&)> CancelWriteCallback;
  typedef std::string ExtensionId;

  Operation(base::WeakPtr<OperationManager> manager,
            const ExtensionId& extension_id,
            const std::string& device_path);

  // Starts the operation.
  void Start();

  // Cancel the operation. This must be called to clean up internal state and
  // cause the the operation to actually stop.  It will not be destroyed until
  // all callbacks have completed.
  void Cancel();

  // Aborts the operation, cancelling it and generating an error.
  void Abort();

  // Informational getters.
  int GetProgress();
  image_writer_api::Stage GetStage();

#if !defined(OS_CHROMEOS)
  // Set an ImageWriterClient to use.  Should be called only when testing.
  void SetUtilityClientForTesting(
      scoped_refptr<ImageWriterUtilityClient> client);
#endif

 protected:
  virtual ~Operation();

  // This function should be overriden by subclasses to set up the work of the
  // operation.  It will be called from Start().
  virtual void StartImpl() = 0;

  // Unzips the current file if it ends in ".zip".  The current_file will be set
  // to the unzipped file.
  void Unzip(const base::Closure& continuation);

  // Writes the current file to device_path.
  void Write(const base::Closure& continuation);

  // Verifies that the current file and device_path contents match.
  void VerifyWrite(const base::Closure& continuation);

  // Completes the operation.
  void Finish();

  // Generates an error.
  // |error_message| is used to create an OnWriteError event which is
  // sent to the extension
  virtual void Error(const std::string& error_message);

  // Set |progress_| and send an event.  Progress should be in the interval
  // [0,100]
  void SetProgress(int progress);
  // Change to a new |stage_| and set |progress_| to zero.  Triggers a progress
  // event.
  void SetStage(image_writer_api::Stage stage);

  // Can be queried to safely determine if the operation has been cancelled.
  bool IsCancelled();

  // Adds a callback that will be called during clean-up, whether the operation
  // is aborted, encounters and error, or finishes successfully.  These
  // functions will be run on the FILE thread.
  void AddCleanUpFunction(const base::Closure& callback);

  // Completes the current operation (progress set to 100) and runs the
  // continuation.
  void CompleteAndContinue(const base::Closure& continuation);

  // If |file_size| is non-zero, only |file_size| bytes will be read from file,
  // otherwise the entire file will be read.
  // |progress_scale| is a percentage to which the progress will be scale, e.g.
  // a scale of 50 means it will increment from 0 to 50 over the course of the
  // sum.  |progress_offset| is an percentage that will be added to the progress
  // of the MD5 sum before updating |progress_| but after scaling.
  void GetMD5SumOfFile(
      const base::FilePath& file,
      int64 file_size,
      int progress_offset,
      int progress_scale,
      const base::Callback<void(const std::string&)>& callback);

  base::WeakPtr<OperationManager> manager_;
  const ExtensionId extension_id_;

  base::FilePath image_path_;
  base::FilePath device_path_;

  // Temporary directory to store files as we go.
  base::ScopedTempDir temp_dir_;

 private:
  friend class base::RefCountedThreadSafe<Operation>;

#if !defined(OS_CHROMEOS)
  // Ensures the client is started.  This may be called many times but will only
  // instantiate one client which should exist for the lifetime of the
  // Operation.
  void StartUtilityClient();

  // Stops the client.  This must be called to ensure the utility process can
  // shutdown.
  void StopUtilityClient();

  // Reports progress from the client, transforming from bytes to percentage.
  virtual void WriteImageProgress(int64 total_bytes, int64 curr_bytes);

  scoped_refptr<ImageWriterUtilityClient> image_writer_client_;
#endif

#if defined(OS_CHROMEOS)
  void StartWriteOnUIThread(const base::Closure& continuation);

  void OnBurnFinished(const base::Closure& continuation,
                      const std::string& target_path,
                      bool success,
                      const std::string& error);
  void OnBurnProgress(const std::string& target_path,
                      int64 num_bytes_burnt,
                      int64 total_size);
  void OnBurnError();
#endif

  // Incrementally calculates the MD5 sum of a file.
  void MD5Chunk(base::File file,
                int64 bytes_processed,
                int64 bytes_total,
                int progress_offset,
                int progress_scale,
                const base::Callback<void(const std::string&)>& callback);

  // Callbacks for zip::ZipReader.
  void OnUnzipFailure();
  void OnUnzipProgress(int64 total_bytes, int64 progress_bytes);

  // Runs all cleanup functions.
  void CleanUp();

  // |stage_| and |progress_| are owned by the FILE thread, use |SetStage| and
  // |SetProgress| to update.  Progress should be in the interval [0,100]
  image_writer_api::Stage stage_;
  int progress_;

  // MD5 contexts don't play well with smart pointers.  Just going to allocate
  // memory here.  This requires that we only do one MD5 sum at a time.
  base::MD5Context md5_context_;

  // Zip reader for unzip operations.
  zip::ZipReader zip_reader_;

  // CleanUp operations that must be run.  All these functions are run on the
  // FILE thread.
  std::vector<base::Closure> cleanup_functions_;
};

}  // namespace image_writer
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_IMAGE_WRITER_PRIVATE_OPERATION_H_
