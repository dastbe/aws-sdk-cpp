/*
  * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  *
  * Licensed under the Apache License, Version 2.0 (the "License").
  * You may not use this file except in compliance with the License.
  * A copy of the License is located at
  *
  *  http://aws.amazon.com/apache2.0
  *
  * or in the "license" file accompanying this file. This file is distributed
  * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
  * express or implied. See the License for the specific language governing
  * permissions and limitations under the License.
  */


#pragma once

#include <aws/core/Core_EXPORTS.h>
#include <aws/core/utils/UnreferencedParam.h>
#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/threading/ReaderWriterLock.h>
#include <aws/core/internal/AWSHttpResourceClient.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/config/AWSProfileConfigLoader.h>
#include <aws/core/client/RetryStrategy.h>
#include <memory>

namespace Aws
{
    namespace Auth
    {
        static int REFRESH_THRESHOLD = 1000 * 60 * 5;

        AWS_CORE_API Aws::String GetConfigProfileFilename(); //defaults to "config"
        AWS_CORE_API Aws::String GetConfigProfileName(); //defaults to "default"

        /**
          * Abstract class for retrieving AWS credentials. Create a derived class from this to allow
          * various methods of storing and retrieving credentials. Examples would be cognito-identity, some encrypted store etc...
          */
        class AWS_CORE_API AWSCredentialsProvider
        {
        public:
            /**
             * Initializes provider. Sets last Loaded time count to 0, forcing a refresh on the
             * first call to GetAWSCredentials.
             */
            AWSCredentialsProvider() : m_lastLoadedMs(0)
            {
            }

            virtual ~AWSCredentialsProvider() = default;

            /**
             * The core of the credential provider interface. Override this method to control how credentials are retrieved.
             */
            virtual AWSCredentials GetAWSCredentials() = 0;

        protected:
            /**
             * The default implementation keeps up with the cache times and lets you know if it's time to refresh your internal caching
             *  to aid your implementation of GetAWSCredentials.
             */
            virtual bool IsTimeToRefresh(long reloadFrequency);
            virtual void Reload();
            mutable Aws::Utils::Threading::ReaderWriterLock m_reloadLock;
        private:
            long long m_lastLoadedMs;
        };

        /**
         * Simply a provider that always returns empty credentials. This is useful for a client that needs to make unsigned
         * calls.
         */
        class AWS_CORE_API AnonymousAWSCredentialsProvider : public AWSCredentialsProvider
        {
        public:
            /**
             * Returns empty credentials object.
             */
            inline AWSCredentials GetAWSCredentials() override { return AWSCredentials("", ""); }
        };

        /**
          * A simple string provider. It takes the AccessKeyId and the SecretKey as constructor args and
          * provides them through the interface. This is the default class for AWSClients that take string
          * arguments for credentials.
          */
        class AWS_CORE_API SimpleAWSCredentialsProvider : public AWSCredentialsProvider
        {
        public:
            /**
             * Initializes object from awsAccessKeyId, awsSecretAccessKey, and sessionToken parameters. sessionToken parameter is defaulted to empty.
             */
            inline SimpleAWSCredentialsProvider(const Aws::String& awsAccessKeyId, const Aws::String& awsSecretAccessKey, const Aws::String& sessionToken = "")
                : m_accessKeyId(awsAccessKeyId), m_secretAccessKey(awsSecretAccessKey), m_sessionToken(sessionToken)
            { }

            /**
            * Initializes object from credentials object. everything is copied.
            */
            inline SimpleAWSCredentialsProvider(const AWSCredentials& credentials)
                : m_accessKeyId(credentials.GetAWSAccessKeyId()), m_secretAccessKey(credentials.GetAWSSecretKey()),
                m_sessionToken(credentials.GetSessionToken())
            { }

            /**
             * Returns the credentials this object was initialized with as an AWSCredentials object.
             */
            inline AWSCredentials GetAWSCredentials() override
            {
                return AWSCredentials(m_accessKeyId, m_secretAccessKey, m_sessionToken);
            }

        private:
            Aws::String m_accessKeyId;
            Aws::String m_secretAccessKey;
            Aws::String m_sessionToken;
        };

        /**
        * Reads AWS credentials from the Environment variables AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY and AWS_SESSION_TOKEN if they exist. If they
        * are not found, empty credentials are returned.
        */
        class AWS_CORE_API EnvironmentAWSCredentialsProvider : public AWSCredentialsProvider
        {
        public:
            /**
            * Reads AWS credentials from the Environment variables AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY and AWS_SESSION_TOKEN if they exist. If they
            * are not found, empty credentials are returned. Credentials are not cached.
            */
            AWSCredentials GetAWSCredentials() override;
        };

        /**
        * Reads credentials profile from the default Profile Config File. Refreshes at set interval for credential rotation.
        * Looks for environment variables AWS_SHARED_CREDENTIALS_FILE and AWS_PROFILE. If they aren't found, then it defaults
        * to the default profile in ~/.aws/credentials.
        * Optionally a user can specify the profile and it will override the environment variable
        * and defaults. To alter the file this pulls from, then the user should alter the AWS_SHARED_CREDENTIALS_FILE variable.
        */
        class AWS_CORE_API ProfileConfigFileAWSCredentialsProvider : public AWSCredentialsProvider
        {
        public:

            /**
            * Initializes with refreshRateMs as the frequency at which the file is reparsed in milliseconds. Defaults to 5 minutes.
            */
            ProfileConfigFileAWSCredentialsProvider(long refreshRateMs = REFRESH_THRESHOLD);

            /**
            * Initializes with a profile override and
            * refreshRateMs as the frequency at which the file is reparsed in milliseconds. Defaults to 5 minutes.
            */
            ProfileConfigFileAWSCredentialsProvider(const char* profile, long refreshRateMs = REFRESH_THRESHOLD);

            /**
            * Retrieves the credentials if found, otherwise returns empty credential set.
            */
            AWSCredentials GetAWSCredentials() override;

            /**
             * Returns the fullpath of the calculated credentials profile file
             */
            static Aws::String GetCredentialsProfileFilename();

            /**
             * Returns the directory storing the profile file.
             */
            static Aws::String GetProfileDirectory();

        protected:
            void Reload() override;
        private:

            /**
            * Checks to see if the refresh interval has expired and reparses the file if it has.
            */
            void RefreshIfExpired();

            Aws::String m_profileToUse;
            Aws::Config::AWSConfigFileProfileConfigLoader m_credentialsFileLoader;
            long m_loadFrequencyMs;
        };

        /**
        * Credentials provider implementation that loads credentials from the Amazon
        * EC2 Instance Metadata Service.
        */
        class AWS_CORE_API InstanceProfileCredentialsProvider : public AWSCredentialsProvider
        {
        public:
            /**
             * Initializes the provider to refresh credentials form the EC2 instance metadata service every 5 minutes.
             * Constructs an EC2MetadataClient using the default http stack (most likely what you want).
             */
            InstanceProfileCredentialsProvider(long refreshRateMs = REFRESH_THRESHOLD);

            /**
             * Initializes the provider to refresh credentials form the EC2 instance metadata service every 5 minutes,
             * uses a supplied EC2MetadataClient.
             */
            InstanceProfileCredentialsProvider(const std::shared_ptr<Aws::Config::EC2InstanceProfileConfigLoader>&, long refreshRateMs = REFRESH_THRESHOLD);

            /**
            * Retrieves the credentials if found, otherwise returns empty credential set.
            */
            AWSCredentials GetAWSCredentials() override;

        protected:
            void Reload() override;

        private:
            void RefreshIfExpired();

            std::shared_ptr<Aws::Config::AWSProfileConfigLoader> m_ec2MetadataConfigLoader;
            long m_loadFrequencyMs;
        };

        /**
        * ECS credentials provider implementation that loads credentials from the Amazon
        * ECS metadata service or an arbitrary endpoint.
        */
        class AWS_CORE_API TaskRoleCredentialsProvider : public AWSCredentialsProvider
        {
        public:
            /**
             * Initializes the provider to retrieve credentials from the ECS metadata service every 5 minutes,
             * or before it expires.
             * @param resourcePath A path appended to the metadata service endpoint.
             * @param refreshRateMs The number of milliseconds after which the credentials will be fetched again.
             */
            TaskRoleCredentialsProvider(const char* resourcePath, long refreshRateMs = REFRESH_THRESHOLD);

            /**
             * Initializes the provider to retrieve credentials from a provided endpoint every 5 minutes or before it
             * expires.
             * @param endpoint The full URI to resolve to get credentials.
             * @param token An optional authorization token passed to the URI via the 'Authorization' HTTP header.
             * @param refreshRateMs The number of milliseconds after which the credentials will be fetched again.
             */
            TaskRoleCredentialsProvider(const char* endpoint, const char* token, long refreshRateMs = REFRESH_THRESHOLD);

            /**
             * Initializes the provider to retrieve credentials using the provided client.
             * @param client The ECSCredentialsClient instance to use when retrieving credentials.
             * @param refreshRateMs The number of milliseconds after which the credentials will be fetched again.
             */
            TaskRoleCredentialsProvider(const std::shared_ptr<Aws::Internal::ECSCredentialsClient>& client,
                    long refreshRateMs = REFRESH_THRESHOLD);
            /**
            * Retrieves the credentials if found, otherwise returns empty credential set.
            */
            AWSCredentials GetAWSCredentials() override;

        protected:
            void Reload() override;
        private:
            bool ExpiresSoon() const;
            void RefreshIfExpired();

        private:
            std::shared_ptr<Aws::Internal::ECSCredentialsClient> m_ecsCredentialsClient;
            long m_loadFrequencyMs;
            Aws::Utils::DateTime m_expirationDate;
            Aws::Auth::AWSCredentials m_credentials;
        };

        /**
         * Process credentials provider that loads credentials by running another command (or program) configured in config file
         * The configuration format is as following:
         * credential_process = command_path <arguments_list>
         * Each time the credentials needs to be refreshed, this command will be executed with configured arguments.
         * The default profile name to look up this configuration is "default", same as normal aws credentials configuration and other configurations.
         * The expected valid output of the command is a Json doc output to stdout:
         * {"Version": 1, "AccessKeyId": "AccessKey123", "SecretAccessKey": "SecretKey321", "SessionToken": "Token123", "Expiration": "1970-01-01T00:00:01Z"}
         * The Version key specifies the version of the JSON payload and must be set to 1 for now (as an integer type). 
         * If the Version key is bumped to 2, SDKs would support both versions of the returned payload.
         * Value of Expiration field should be an valid ISO8601 formatted date string as above example.
         * The expected error message of the command is a string to output to stderr.
         */
        class AWS_CORE_API ProcessCredentialsProvider : public AWSCredentialsProvider
        {
        public:
            /**
             * Initializes the provider by checking default profile
             */
            ProcessCredentialsProvider();

            /**
             * Initializes the provider by checking specified profile
             * @param profile which profile in config file to use.
             */
            ProcessCredentialsProvider(const Aws::String& profile);

            /**
             * Retrieves the credentials if found, otherwise returns empty credential set.
             */
            AWSCredentials GetAWSCredentials() override;

        protected:
            void Reload() override;
        private:
            void RefreshIfExpired();

        private:
            Aws::String m_profileToUse;
            Aws::Config::AWSConfigFileProfileConfigLoader m_configFileLoader;
            Aws::Auth::AWSCredentials m_credentials;
            Aws::Utils::DateTime m_expire;
        };
    } // namespace Auth
} // namespace Aws
